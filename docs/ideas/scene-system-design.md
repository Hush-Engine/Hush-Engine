# Scene System Design

## Context

The engine currently has a single `Scene` per `IApplication` (`src/editor/.../EditorApp.cpp:50`), wrapping a Flecs ECS world (`src/engine_core/core/src/Scene.hpp:44`). There is no way to save/load scenes to disk, and no scene-switching mechanism. This design adds:

- `HushEngine::NewScene()` — create a scene and make it the engine's active scene
- `ResourceManager::LoadScene(path)` — load a scene file (e.g. `res://lvl1.scene`)

The engine already has a mature serialization stack (RapidJSON + concept-based framework + `hush-reflection` code generator emitting `HUSH_GENERATED_BODY`), so the scene format leans on this rather than introducing a new toolchain.

## Target API

```cpp
// Empty scene
Hush::Scene* s = engine.NewScene();

// From file — two-step: load asset, instantiate
auto assetRes = engine.GetResourceManager()->LoadScene("res://levels/lvl1.scene");
Hush::Ref<Hush::SceneAsset> asset = assetRes.value();
Hush::Scene* scene = engine.NewScene(asset);  // becomes the active scene

// Swap
Hush::Scene* other = engine.NewScene(otherAsset);  // replaces active; old one is shut down

// Reload
engine.UnloadScene(scene);
Hush::Scene* fresh = engine.NewScene(asset);  // asset still cached in ResourceManager
```

**Semantics:**
- `LoadScene` returns a cached `Ref<SceneAsset>` (file content), not a live `Scene`.
- `NewScene(asset)` instantiates the asset into a fresh `Scene`.
- Engine holds **one** active scene. Swap replaces. Additive loading is out of scope (for now).

## Scene File Format (JSON)

```json
{
  "format_version": 1,
  "entities": [
    {
      "uuid": "a3f1c4d2-...",
      "name": "Player",
      "parent": null,
      "components": [
        { "__type": "Hush::Transform", "position": {...}, "rotation": {...}, "scale": {...} },
        { "__type": "Hush::MeshRenderer", "mesh": "res://models/player.glb" }
      ]
    },
    {
      "uuid": "b7c2e8a1-...",
      "name": "Weapon",
      "parent": "a3f1c4d2-...",
      "components": [ ... ]
    }
  ]
}
```

**Why JSON:**
- RapidJSON already in `vcpkg.json`
- `JsonSerializer` (`src/engine_core/core/src/serialization/Formats/JsonSerializer.hpp:26`) already emits component data with `__type` tag
- `hush-reflection.exe` already generates per-component Serialize/Deserialize via `HUSH_GENERATED_BODY` (`src/engine_core/core/src/Hushgen.hpp:11`)
- Human-readable, git-diff-friendly. Binary cooker can be layered on later — do not build speculatively.

**Key format decisions:**
- Persistent entity **UUIDs** (string form) — stable across saves, supports future prefab reuse, editor reshuffling, merges.
- Asset references (mesh, texture paths) stored as `res://` strings; resolved lazily by the component's deserializer when it calls `ResourceManager::Load*`.
- Parent references by UUID, resolved in a second pass after all entities exist.

## Pieces to Build

### 1. Prerequisite: `res://` scheme in VFS
- **File:** `src/engine_core/filesystem/src/VirtualFilesystem.cpp/.hpp`
- Strip `res://` prefix and route to the project's resource mount. Small change; touches path normalization only. Required by both `LoadScene` and component-level asset refs.

### 2. `SceneAsset` — new resource type
- **New file:** `src/engine_core/resources/src/SceneAsset.hpp/.cpp` (alongside other cached resources)
- One `rapidjson::Document` owns the parsed file buffer; `EntityData` / `ComponentData` are structured views into it. Owning `std::string` fields for the small bits (uuid, name) keep lifetime reasoning simple; the bulk component JSON is a non-owning `const rapidjson::Value*` into the same Document.
- Immovable and non-copyable once built: it lives behind a `Ref<SceneAsset>` in the ResourceManager cache and the internal pointers must stay stable.
- Annotated `[[hush::export, hush::reflect]]` and carrying `HUSH_GENERATED_BODY` — same pattern as `Transform` (`src/engine_core/core/src/Components/Transform.hpp:12-24`). This registers the type with `ReflectionDB` and lets the codegen emit `Serialize`/`Deserialize` for the trivially-reflectable fields (`m_sourcePath`, `m_formatVersion`, `m_entities`). `ComponentData` is the exception — it holds an opaque `rapidjson::Value*` the generator cannot reflect, so it supplies a hand-written `Serialize`/`Deserialize` pair instead of the macro. See the "ComponentData is the special case" note below.

```cpp
#pragma once

#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <Hushgen.hpp>

#if __has_include("SceneAsset.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "SceneAsset.hushgen.hpp"
#endif

#include <rapidjson/document.h>
#include <atomic>
#include <optional>
#include <string>
#include <vector>

namespace Hush
{
    struct [[hush::export, hush::reflect]] ComponentData
    {
        // No HUSH_GENERATED_BODY — the `data` pointer is opaque to the codegen.
        // Serialize/Deserialize are hand-written (see SceneAsset.cpp).
        std::string              typeName;   // mirrors "__type" from JSON
        const rapidjson::Value  *data;       // non-owning view into SceneAsset::m_document

        template <typename T>
        Hush::Serialization::ESerializationError Serialize(T &serializer) const;
        auto Deserialize(Hush::Serialization::IVisitor *parent,
                         Hush::Serialization::EFormatDescribingType format);
    };

    struct [[hush::export, hush::reflect]] EntityData
    {
        HUSH_GENERATED_BODY
    public:
        std::string                 uuid;           // canonical UUIDv4 string
        std::optional<std::string>  parentUuid;
        std::string                 name;
        std::vector<ComponentData>  components;
    };

    class [[hush::export, hush::reflect]] SceneAsset
    {
        HUSH_GENERATED_BODY
    public:
        static Result<std::unique_ptr<SceneAsset>, EError> FromJsonStream(
            std::string sourcePath,
            std::istream &input);

        SceneAsset(const SceneAsset &) = delete;
        SceneAsset &operator=(const SceneAsset &) = delete;
        SceneAsset(SceneAsset &&) = delete;
        SceneAsset &operator=(SceneAsset &&) = delete;

        std::string_view                SourcePath() const noexcept    { return m_sourcePath; }
        uint32_t                        FormatVersion() const noexcept { return m_formatVersion; }
        const std::vector<EntityData>  &Entities() const noexcept      { return m_entities; }

        // Hot-reload hook (see "Evaluated Extensions" below). Bumped by the
        // ResourceManager when the file is reloaded; Scene instances spawned
        // from this asset can compare against their captured value to know
        // they're stale.
        uint64_t Generation() const noexcept { return m_generation; }

    private:
        SceneAsset(std::string sourcePath, rapidjson::Document doc, uint32_t version);

        std::string             m_sourcePath;
        uint32_t                m_formatVersion = 1;
        rapidjson::Document     m_document;    // owns the parsed JSON buffer
        std::vector<EntityData> m_entities;    // views into m_document
        std::atomic<uint64_t>   m_generation{0};

        friend class ResourceManager;          // reloads bump m_generation / rebuild
    };
} // namespace Hush
```

**Why this shape:**
- Single `rapidjson::Document` avoids per-component parse allocations and keeps deserialization fast — the `hush-reflection`-generated `Deserialize` walks the same rapidjson value tree the rest of the engine already uses.
- Structured `EntityData` vector lets the instantiate step do a straightforward two-pass (create → resolve parent) without re-walking the raw JSON.
- Non-copyable/non-movable is important: `ComponentData::data` is a raw pointer into `m_document`, so moves would dangle. `Ref<SceneAsset>` already handles sharing.

**`ComponentData` is the special case.** The codegen reflects fields by type, but `const rapidjson::Value*` isn't a type the generator knows how to serialize. `ComponentData` therefore opts out of `HUSH_GENERATED_BODY` and defines its own `Serialize` / `Deserialize`:
- `Deserialize` pulls the `__type` string from the current object and records the object's `rapidjson::Value*` pointer as `data` — no recursion into component fields at this stage. That recursion happens later, at `NewScene(asset)` time, when the target component type is known and its reflection-generated `Deserialize` can be invoked against `data`.
- `Serialize` (editor save path, future) writes `*data` straight to the serializer since it already carries the component's own `__type` prefix from when it was originally emitted.

This keeps the scene load path uniform with the rest of the engine's reflection-based deserialization, while isolating the one piece of polymorphic/opaque data to a single hand-written file.

### 3. `ResourceManager::LoadScene(path)` — mirror `LoadTexture`
- **File:** `src/engine_core/resources/src/ResourceManager.hpp:82` (add near `LoadTexture`) and `ResourceManager.cpp:61` (mirror the same caching pattern)
- Steps: `Fnv1a64` hash the path → lookup in `m_loadedResources` → if miss, `m_filesystem->OpenFile(path)` → parse via `JsonSerializer` / `Deserialization.hpp` visitor into `SceneAsset` → cache as `Ref<SceneAsset>`.
- Return `Result<Ref<SceneAsset>, EError>`.

### 4. `HushEngine::NewScene()` and `NewScene(Ref<SceneAsset>)`
- **File:** `src/engine_core/src/HushEngine.hpp:23` (add), `HushEngine.cpp`
- `NewScene()`: construct a new `Scene`, shut down previous active, install new one as active, return pointer.
- `NewScene(Ref<SceneAsset> asset)`:
  1. Construct empty `Scene`.
  2. For each `EntityData`: create entity via `Scene::CreateEntity()` (`Scene.cpp:173`), store UUID→entity map.
  3. For each component in the `EntityData`: look up type in `ReflectionDB`, use in-place constructor + deserializer (see `Type.hpp:33`) to attach the component.
  4. Second pass: resolve parent links via the UUID map.
  5. Install as active scene; return pointer.
- `UnloadScene(Scene*)` / `SetActiveScene(Scene*)`: swap helpers. Old scene is `Shutdown()`-ed.

### 5. Move scene ownership: `IApplication` → `HushEngine`
- Currently `EditorApp` owns `m_scene` (`src/editor/.../EditorApp.cpp:50`). Engine exposes it via `HushEngine::GetScene()` (`HushEngine.cpp:181`).
- After the change: `HushEngine` owns the active scene as `std::unique_ptr<Scene> m_activeScene`. Application's first-scene setup moves into an `OnInit` callback that calls `engine.NewScene(...)`.
- Update `EditorApp` accordingly.

### 6. Entity identity: UUID + name storage
- Add a `Uuid` (16 bytes, string-formatted for JSON) and `Name` (`std::string`) component — or store both on a Scene-side map keyed by `EntityId` so we don't force every entity to carry them at runtime.
- Lean toward the map approach: keeps runtime memory tight; only entities that came from a file (or are saved) need a UUID.
- UUIDv4 generator utility: one-off in `src/engine_core/core/src/Utils/` (no new dep — `std::random_device` + 128 bits).

## Reused Infrastructure (do not re-implement)

- `JsonSerializer` (`src/engine_core/core/src/serialization/Formats/JsonSerializer.hpp:26`)
- `IVisitor` deserialization (`src/engine_core/core/src/serialization/Deserialization.hpp:45`)
- `HUSH_GENERATED_BODY` reflection macro (`src/engine_core/core/src/Hushgen.hpp:11`)
- `ReflectionDB` type registry (`src/engine_core/core/src/Type.hpp:33`)
- `Ref<T>` + FNV-1a path cache pattern (copy from `LoadTexture` in `ResourceManager.cpp:61`)
- `Scene::CreateEntity` (`Scene.cpp:173`), `RegisterIfNeededSlow` (`Scene.hpp:269`)

## Out of Scope (explicitly deferred)

- Additive / multi-scene loading (Unity-style).
- Editor "Save Scene" UI.
- Prefabs and nested scenes — evaluated below.
- Hot-reload of `.scene` files — evaluated below.
- Binary cooked format — evaluated below.

## Evaluated Extensions

Two extensions were considered and deliberately deferred. Both remain viable follow-ups; the first-cut design is shaped so they plug in without rewrites.

### Hot Reload

**Current state.** `ResourceManager` caches by path hash and only evicts on refcount drop (`ResourceManager.hpp:126`). There is no file watcher, no mtime check, no reload path for any resource type.

**Value if added.** Large. "Edit scene file → see it live in the editor without restarting" is the single biggest dev-quality-of-life win for scene work, and it's one of the main reasons Godot's `res://` + `.tscn` workflow feels good.

**Two tiers of ambition:**

1. **Asset-level reload (cheap).** On file change, re-parse the `SceneAsset` in place and bump `m_generation`. Any live `Scene` that was instantiated from it is *not* touched automatically, but the user can click "Reload Scene" in the editor and `NewScene(asset)` picks up the new content. Implementation cost: a file watcher (`ReadDirectoryChangesW` on Windows, `inotify` on Linux, `FSEvents` on mac) plus ~50 LOC in the ResourceManager to invalidate + re-parse. Editor button is trivial since `UnloadScene` + `NewScene(asset)` already exists in this design.

2. **Live-diff reload (expensive).** Re-parse the asset, diff against the currently running `Scene` (UUIDs make this feasible), and apply only the changes — add new entities, remove deleted ones, update changed components, leave runtime-mutated state alone. This is real editor-grade live authoring. Cost: a full entity/component diff pass, merge policy for fields that were mutated at runtime (whose values win?), and careful testing. Probably 1–2 weeks of focused work.

**Recommendation.** Pre-wire Tier 1 from day one: the `m_generation` counter is already in the class above, and the reload path is just "rebuild `m_document` and the `EntityData` vector in place." Actually *enabling* reload waits on a file-watcher abstraction, which is a small, self-contained follow-up PR. Tier 2 is a separate initiative and should be weighed against just making the Tier 1 reload button fast enough that no one misses live-diff.

**Concrete hooks added to the first cut for this:**
- `SceneAsset::Generation()` accessor (present in the class above).
- Route scene parsing through a single `FromJsonStream` entry so the same code path handles first-load and reload.
- Design `NewScene(asset)` to read the asset only during instantiation — no lingering references into `SceneAsset` internals from the live `Scene` — so reloading the asset never corrupts a running scene.

### Binary Cooked Format

**When it matters.** Load-time budget and ship size. With RapidJSON (~100 MB/s parse), a 100 KB scene deserializes in under a millisecond; even a 1 MB scene is ~10 ms. For typical game scenes that's fine. The case for binary becomes real when:
- Scenes grow into the MB range (dense prefab trees, baked navigation data stored inline, etc.).
- Ship builds need to strip editor-only fields (names, UUIDs, debug tags) for size and mild anti-tamper.
- Startup-critical scenes need sub-millisecond load.

**Options, ranked by fit:**

1. **Second `Serializer` backend (recommended if we ever do this).** The existing framework is already format-agnostic — `JsonSerializer` is one concrete `Serializer` concept implementation (`src/engine_core/core/src/serialization/Formats/JsonSerializer.hpp:26`). Add a sibling `BinarySerializer` with the same primitives (int8..int64, float, double, string, array, object-as-tagged-fields). The `hush-reflection` generator already calls through the `Serializer` concept, so every component that currently serializes to JSON gets binary for free. Biggest win per unit of work.

2. **FlatBuffers / Cap'n Proto.** Zero-copy reads, schema evolution, battle-tested. Downsides: new dependency, schema files to maintain in parallel with the reflection metadata, and the existing codegen pipeline doesn't emit FlatBuffers schemas. Would replace, not extend, the current stack for scenes specifically — not worth the split-brain.

3. **MessagePack.** Cheap to add, modestly faster than JSON, still has per-field parse overhead. Doesn't meaningfully beat option 1 and adds a dep.

**Pipeline if we go with option 1.**
- Author: `foo.scene` (JSON), hand-written or editor-saved.
- Cook step: offline tool (`hush-cook` or an extension of `hush-reflection`) loads the JSON scene via `JsonSerializer` and re-emits it via `BinarySerializer` to `foo.scene.bin`.
- Runtime: `ResourceManager::LoadScene` dispatches on extension (`.scene` → JSON path, `.scene.bin` → binary path). Same `SceneAsset` result either way.

**Recommendation.** Don't build now. Leave the door open with two cheap decisions:
- Dispatch inside `LoadScene` by extension so adding a binary path later is additive.
- Keep scene loading behind `SceneAsset::FromJsonStream` (a named entry point) rather than a single monolithic function — a future `FromBinaryStream` slots in next to it.

Revisit when a real scene's JSON load time shows up in a profile. Until then, JSON covers authoring, diffing, and load-time at once with no extra surface.

### Nested Scenes (Prefabs)

**What it is.** The ability to reference one `.scene` file from inside another — author `Player.scene` once, drop instances of it into `Level1.scene`, `Level2.scene`, etc. Edits to `Player.scene` propagate to every level that uses it; per-instance overrides let a specific placement differ (spawn position, starting weapon, …). This is Godot's instanced scenes, Unity's prefabs, Unreal's blueprint-as-asset.

**Value.** High for content workflow. Without it, designers copy-paste entity trees and divergence compounds fast. With it, a single file is the source of truth for an enemy, pickup, UI panel, room chunk.

**What's already in place.** Most of the foundation is free:
- **UUIDs** give stable targets for overrides.
- **`res://` paths** mean a nested scene reference is just another path string.
- **`ResourceManager` cache** gives free prefab reuse — instancing `Player.scene` twice in the same level parses the file once.
- **Two-phase load/instantiate** means sub-scene instantiation is literally the same code path as top-level instantiation, called recursively.

**What would need to be added:**

1. **File-format extension — a distinct node kind for scene instances.** Cleanest as a separate top-level array, not mixed with plain entities:
   ```json
   {
     "entities": [ ... ],
     "scene_instances": [
       {
         "uuid": "c9d2-...",
         "source": "res://prefabs/player.scene",
         "parent": null,
         "overrides": [
           {
             "target": "<prefab-local-uuid>",
             "components": [
               { "__type": "Hush::Transform", "position": [12, 0, 5] }
             ]
           }
         ]
       }
     ]
   }
   ```

2. **UUID composition.** Instancing the same prefab twice in a level means prefab-local UUIDs collide. Two approaches:
   - *Re-UUID on instantiation*: mint fresh UUIDs for every instanced entity. Simple, but overrides must be keyed by prefab-local UUID and translated through an instance-local map at load time.
   - *Composite UUID*: runtime UUID = `hash(instance_uuid, prefab_local_uuid)`. Deterministic, overrides work without translation, nested-in-nested composes naturally. Unity and Unreal handle prefab instance identity roughly this way.
   - The current design uses `std::string` for UUIDs, which accommodates either form without touching the class.

3. **Cycle detection.** Prefab A → Prefab B → Prefab A recurses forever. Track visited `res://` paths on the instantiation stack inside `NewScene(asset)`; error on revisit.

4. **Override application.** After base prefab instantiation, walk each override entry and apply partial component data to the target entity. The reflection-generated `Deserialize` already handles partial data naturally — only the fields present in the JSON are touched — so "apply override" = "deserialize component subtree against an existing component instance." No new deserialization machinery needed.

5. **Editor UX (separate, significant).** Instance-as-one-selection, break-prefab-link, save-overrides-back-to-source, editing a prefab in isolation. Critical for authoring, but orthogonal to runtime support.

**Design decisions worth locking in now, even without building:**
- **Keep UUID as `std::string`** in `EntityData` — accommodates composite forms later without breaking the class or the file format.
- **Reserve `scene_instances` as an optional top-level key** in the format. First-cut loader ignores it; future loader parses it. Non-breaking for files authored today.
- **`NewScene(asset)` is the recursion boundary, not `LoadScene`.** `LoadScene` stays a pure file → `SceneAsset` operation (and its cache trivially becomes prefab reuse); recursive instantiation lives in `NewScene(asset)`. Keeps loading and instantiation cleanly separated and keeps cycle-detection state localized to the instantiation call.

**Recommendation.** Don't build. Lock in the three decisions above so the file format, the `SceneAsset` class, and the `NewScene` signature don't need breaking changes when prefabs arrive. Overrides and UUID composition are the hard parts — they deserve their own design pass when prefab work is prioritized, and the editor UX is the larger investment in that effort, not the loader.

## Open Design Questions

1. **Scene-scoped vs. engine-scoped systems.** Today `Scene` owns its systems (`Scene.hpp:287`). If swap shuts them down, every swap re-inits systems — fine for most cases but expensive for a heavy renderer. Alternative: split "engine systems" (persist across scenes, e.g. renderer, input) from "scene systems" (per-scene, torn down on swap). Recommend splitting *later*, not in the first cut — keep systems scene-owned for now, flag as a near-term follow-up.
2. **SceneAsset storage: parsed-DOM view vs. pre-typed components.** Two ways to represent components between "file loaded" and "scene instantiated":
   - *Parsed-DOM view* (recommended, what the class above uses): keep the `rapidjson::Document` live, and store a `const rapidjson::Value*` per component pointing into its subtree. **"Raw JSON" here means the in-memory rapidjson node tree — not the unparsed text of the file, and not a re-serialized string.** The expensive work (text → DOM) is done once at load; the typed conversion (DOM → `Hush::Transform`) happens per-entity at `NewScene(asset)` time.
   - *Pre-typed components*: walk the DOM during load and eagerly construct each component's C++ object (`Hush::Transform`, `Hush::MeshRenderer`, …), storing a type-erased handle in `ComponentData`. Faster to re-instantiate, but couples the asset cache to the current reflection DB — a component rename or removal invalidates every cached `SceneAsset`.

   Parsed-DOM view is simpler, lazier, and more tolerant of DB changes. Revisit if instantiate latency becomes a real cost.
3. **Eager vs. lazy asset loading during `NewScene(asset)`.** Lazy (component deserializers call `LoadTexture` etc. during instantiate) is the path of least resistance and matches current `LoadTexture` behavior. Can add an eager prefetch pass later if startup stutter is a concern.

## Verification

- **Round-trip test (unit):** build a `Scene` in code, serialize to JSON, write to disk, `LoadScene` back, instantiate, compare entity/component state. Add to `src/engine_core/core/tests/` alongside `Serialization.test.cpp`.
- **End-to-end (editor):** author a simple `res://test.scene` by hand with 2–3 entities + a `Transform` + a `MeshRenderer` → run the editor → confirm entities appear with correct transforms and the mesh asset resolves via `res://`.
- **Swap test:** load scene A, then load scene B, confirm A is torn down (no lingering entities, systems re-init cleanly, no leaks per the `Ref` refcounts).
- **Reload test:** `UnloadScene` → `NewScene(sameAsset)` → confirm `ResourceManager` cache hit (no re-parse) and fresh entities.
