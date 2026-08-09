# Hush Cooker — Design Spec & Implementation Plan (v2)

> Consolidated plan. Merges the architecture of `hush-cooker.md` (module split, format
> completeness, vertical-slice phasing) with the depth of `hush-cooker-ds.md` (per-step phase
> tables, shader thread-safety, multi-backend shaders, BCn, cooked-first VFS, docs plan,
> Emscripten handling). Tracking issue: [#136](https://github.com/Hush-Engine/Hush-Engine/issues/136).

## Overview

Hush Cooker transforms raw authoring assets (PNG/JPEG, `.slang`, glTF) into engine-optimized binary
blobs so the runtime never decodes PNG or compiles shaders on the fly. It runs in **two modes**:

1. **Standalone** (`hush-cooker` CLI, invoked by CMake): packs declared resources into a single
   mmap-able bundle shipped next to the binary.
2. **Embedded** (linked into `HushEditor`): drop a file → create `{name}.{ext}.hmeta` → cook into
   `.hcooked/` → re-cook on change (file watcher) → delete cooked output when `.hmeta` is removed →
   hide `.hmeta` in the Project view.

Today this work happens live: `ResourceManager::LoadTexture` runs `stbi_load_from_memory` per load
(`resources/src/ResourceManager.cpp`) and `RenderingSystem` compiles `.slang` on load via
`ShaderCompiler::CompileFromSource` (`editor/src/systems/RenderingSystem.cpp`). The cooker moves both offline.

### Confirmed scope decisions

| Decision | Choice |
|---|---|
| Sequencing | **Vertical slice through both modes**, one asset type end-to-end at a time |
| First asset types | **Textures + Shaders** (meshes deferred; fastgltf loaders are commented out today) |
| Compression | **zstd, added now** (per-asset, configurable in `.hmeta`) |
| Editor re-cook trigger | **Real file watcher** — vcpkg `thomasmonkman-filewatch` (cross-platform, wraps `ReadDirectoryChangesW`/inotify/FSEvents) |
| Metadata serialization | Existing `Serialization::SerializeJson` (RapidJSON visitor) |
| GPU texture compression (BCn) | Fast-follow after the RGBA8 slice, via vendored `bc7enc` (issue calls out DXT) |

There is half-built scaffolding to fold in, not fight: `ContentPanel::GenerateMetaFiles /
MakeMetaFile / CreateInnerResources` (`editor/src/ContentPanel.cpp:141-241`, currently commented) and
`FileMetadata` (`filesystem/src/FileMetadata.hpp`).

---

## Naming scheme

| Artifact | Meaning | Magic |
|---|---|---|
| `{name}.{ext}.hmeta` | JSON sidecar per source (`hero.png.hmeta`): cook instructions + staleness. Human-editable, hidden in editor. Keeping the source ext avoids `hero.png` vs `hero.gltf` collisions. | — |
| `.hasset` (HAsset) | One cooked binary: header + optional format-extra + (zstd) payload. Lives in `.hcooked/`. | `'HAST'` |
| `.hshader` (HShader) | One cooked shader: multi-backend container (WGSL now; SPIR-V/DXIL slots reserved). | `'HSHD'` |
| `.hcooked/` | Per-project folder of loose cooked outputs (editor cache; gitignored, never committed). | — |
| `.hushpak` (HushPak) | Shipped bundle = header + directory + string table + many HAsset/HShader blobs; mmap-able. | `'HPAK'` |

> **Confirmed:** `.hasset` = a single cooked asset (the issue's `HAsset`), `.hushpak` = the bundle
> (the issue's `HushPak`).

Endianness: **little-endian canonical** (all targets are LE). Write magic + version; defer byte-swap.

---

## Architecture — module & target split

Split the **write path** (heavy: Slang, stb, bc7enc, zstd-compress) from the **read path** (lean:
mmap + zstd-decompress) so the runtime never links the cook toolchain, and so the **shipped
standalone game (`HushRuntime`/examples) — not just the editor — can mount and read bundles.**

```
                 ┌──────────────────────────────┐
                 │  Hush::Assets  (engine_core)  │  READ path — in HushLibs, runtime-linked
                 │  formats · enums · HMeta      │  HAsset/HushPak/HShader codecs,
                 │  PakFileSystem · zstd DECODE  │  read-only mmap IFileSystem, decompress
                 └───────────────┬──────────────┘
             ┌───────────────────┴───────────────────┐
   ┌─────────▼──────────┐                   ┌─────────▼──────────┐
   │  Hush::Cooker (lib) │ WRITE path        │  HushRuntime /     │ mounts .hushpak,
   │  src/cooker/        │ NOT in HushLibs   │  examples          │ loads cooked assets
   │  ICooker + writers  │ (editor+CLI only) └────────────────────┘
   │  stb·bc7enc·slang·  │
   │  zstd ENCODE        │────────┬───────────────────┐
   └─────────────────────┘        │                   │
                          ┌───────▼────────┐  ┌────────▼─────────┐
                          │ HushCookerCli  │  │ HushEditor       │ drop→hmeta→hcooked,
                          │ (hush-cooker)  │  │ + FileWatcher    │ watcher, hide .hmeta
                          └────────────────┘  └──────────────────┘
```

- **`Hush::Assets`** — NEW engine_core module `src/engine_core/assets/`, added to the `HushLibs`
  aggregate (`engine_core/CMakeLists.txt`). Runtime + shared. Links `Hush::Filesystem`, `Hush::Core`
  (serialization), `zstd`. **This is the fix for `-ds`'s biggest structural bug** (it placed
  `PakFileSystem` in `editor/src/` but mounted it from `engine_core`'s `HushEngine.cpp`, which the
  editor is downstream of — so a non-editor runtime could not compile or load its own bundle).
- **`Hush::Cooker`** — NEW **top-level** lib `src/cooker/`, **explicitly NOT** added to `HushLibs`
  (every engine_core module is auto-aggregated into `Hush::Engine`; keeping the cooker top-level
  keeps Slang/stb/bc7enc/zstd-encode out of every runtime binary). Links `Hush::Assets`,
  `Hush::Rendering` (for `ShaderCompiler`), `stb`, `bc7enc`, `slang`, `zstd`.
- **`HushCookerCli`** — NEW exe (binary `hush-cooker`) in `src/cooker/`. Hand-rolled arg parsing
  (no new dep) → `Hush::Cooker`. `hush_deploy_runtime_dlls`.

Use the existing `hush_add_library` / `hush_add_executable` macros (`src/cmake/utils.cmake`) and the
`HushXxx` + `Hush::Xxx` alias convention. `find_package(zstd CONFIG REQUIRED)` goes in
`src/engine_core/cmake/deps.cmake`; `"zstd"` + `"thomasmonkman-filewatch"` added to `vcpkg.json`.

> Known cost: the CLI links `Hush::Rendering` (→ SDL3/WebGPU) for `ShaderCompiler` — acceptable for a
> build-time tool. A later refactor could split `ShaderCompiler` into a lighter target; out of scope now.

---

## On-disk formats (`Hush::Assets`)

The issue's structs use flexible-array members + `size_t` (non-portable). Replace with fixed-width,
explicitly offset, 8-byte-aligned layouts. **Every cooked blob carries both compressed and
uncompressed sizes** — `-ds`'s format stored only the uncompressed size, which leaves a zstd reader
under mmap with no way to bound the compressed frame without `ZSTD_findFrameCompressedSize`.

```
HAssetHeader (40 B, 8-aligned):
  u32 magic 'HAST'; u16 headerVersion=1; u16 flags(bit0=encrypted,reserved);
  u32 format(EAssetFormat); u32 compression(ECompressionFormat);
  u64 uncompressedSize; u64 compressedSize; u32 extraSize; u32 contentHash(Fnv1a of payload)
  [ extra[extraSize] ]        // e.g. HTextureExtra{ u32 w,h,depth,mipCount,arrayLayers,gpuFormat }
  [ payload[compressedSize] ] // pad to 8

HushPakHeader (64 B): u32 magic 'HPAK'; u16 version=1; u16 flags; u32 entryCount; u32 _;
  u64 directoryOffset; u64 stringTableOffset,stringTableSize; u64 dataOffset,totalSize; u8 _[8]
PakEntry (fixed 48 B — array is contiguous & binary-searchable in place, sorted by nameHash):
  u64 nameHash(Fnv1a64(vpath)); u32 nameOffset,nameLength; u64 dataOffset,dataSize;
  u32 format,compression; u64 uncompressedSize
```

Fixed-size `PakEntry` + a separate string table keeps the directory mmap-indexable (O(log n) lookup),
versus `-ds`'s variable-length inline names that force a full parse into a linear-scan vector at open.
Each pak entry's data region is a **complete HAsset/HShader blob**, so a loose `.hasset` file is
byte-identical to its pak slice. Codecs live in `Hush::Assets` (`HAsset.{hpp,cpp}`,
`HushPak.{hpp,cpp}`, `HShader.{hpp,cpp}`) as plain byte read/write. Unit-tested round-trip.

### `.hshader` — multi-backend shader container (adopted from `-ds`, scoped to WGSL now)

Cook once, reserve slots for multiple GPU backends, pick at load time. The runtime is WebGPU/WGSL-only
today (`VulkanLoader` is commented out), so **populate WGSL now; leave SPIR-V/DXIL slots empty** until
a Vulkan/D3D backend exists.

```
HShaderHeader: u32 magic 'HSHD'; u16 version=1; u16 backendCount; u32 totalSize
BackendEntry[backendCount]: u32 backendType; u32 stageCount; u64 dataOffset,dataSize
  (per backend) StageEntry[stageCount]: u32 stage; u32 entryNameLen; char entry[]; u64 off,size
  (per backend) raw bytecode (SPIR-V binary) / text (WGSL)
enum class EShaderBackend : uint32_t { WebGPU_WGSL=0, Vulkan_SPIRV=1, D3D12_DXIL=2 };
```

### `EAssetFormat` / `ECompressionFormat` enums

```cpp
enum class EAssetFormat : uint32_t {
  Unknown=0x00,
  RGBA8_UNORM=0x01, BGRA8_UNORM=0x02, R8_UNORM=0x03, RG8_UNORM=0x04, RGBA16_FLOAT=0x05, R32_FLOAT=0x06,
  DXT1=0x10 /*BC1*/, DXT3=0x11, DXT5=0x12, BC4=0x13, BC5=0x14, BC7=0x15,   // Phase 5 (bc7enc)
  Shader=0x20, Mesh=0x30 /*future*/ };
enum class ECompressionFormat : uint32_t { None=0, Zstd=1 };
```

This is a **new** enum for cooked/GPU-facing formats (mirroring `Graphics::ETextureFormat`), kept
separate from `EFileExtension`, which stays for source-type routing. Add `SLANG`, `HMETA`, `HASSET`,
`HSHADER`, `HUSHPAK` to `EFileExtension` (`filesystem/src/IFile.hpp:32`) and
`IFileSystem::ToKnownExtension` (`filesystem/src/FileSystem.hpp`).

---

## `.hmeta` schema (JSON via existing serializer)

Extends the `FileMetadata` idea into a cook-instruction sidecar. Reuse
`Serialization::SerializeJson<T>` / `DeserializeJson<T>`. **(De)serialization is hand-written** (like
`FileMetadata`), not `[[hush::reflect]]`-generated — the reflection binaries auto-download only on
`CMAKE_HOST_WIN32` (`src/cmake/utils.cmake`), and the CLI must build on Linux/macOS CI.

```json
{ "version": 1, "id": 15963102, "assetType": "texture",
  "outputFormat": 1, "compression": "zstd",
  "importSettings": { "sRGB": true, "generateMipmaps": true, "gpuCompression": "none" } }
```

```cpp
struct HMeta {
  static constexpr uint16_t VERSION = 1;
  uint16_t version = VERSION;
  uint32_t id;                 // Fnv1a(sourceVPath) — carries FileMetadata.id forward
  uint64_t sourceHash;         // Fnv1a64(source bytes) — content staleness
  uint64_t sourceMTime;        // stat mtime — cheap pre-check
  std::string  assetType;      // "texture" | "shader" | "model" | "unknown"
  EAssetFormat outputFormat; ECompressionFormat compression;
  struct TextureSettings { bool sRGB=true; bool generateMipmaps=true;
                           std::string gpuCompression="none"; /*none|BC1|BC3|BC5|BC7*/ uint32_t maxSize=0; };
  struct ShaderSettings  { std::vector<EShaderBackend> backends{EShaderBackend::WebGPU_WGSL};
                           std::vector<std::string> entryPoints; std::vector<std::string> defines; };
  struct ModelSettings   { bool importMaterials=true; bool generateLods=false; float scaleFactor=1.f; };
  TextureSettings texture; ShaderSettings shader; ModelSettings model; // one used, per assetType
  template<class S> Serialization::ESerializationError Serialize(S&) const;  // hand-written
  // + hand-written Deserialize visitor so DeserializeJson<HMeta> works cross-platform.
};
```

---

## Core cooker API (`src/cooker/src/`)

```cpp
class ICooker {
  virtual std::span<const EFileExtension> SupportedExtensions() const = 0;
  virtual bool  CanCook(const FileInfo&) const = 0;
  virtual HMeta DefaultMeta(EFileExtension, std::string_view srcVPath) const = 0;   // seed a fresh .hmeta
  virtual Result<std::vector<std::byte>, ECookError>                                 // one cooked blob
          Cook(std::span<const std::byte> input, const HMeta&, const CookContext&) = 0;  // UNCOMPRESSED
};
class AssetCooker {                      // single reusable entry point for editor + CLI
  void RegisterBuiltins();               // Image + Shader (+ Model later)
  ICooker* FindCooker(EFileExtension) const;
  Result<std::vector<std::byte>,ECookError> CookToBlob(...);   // runs cooker, applies meta.compression (zstd)
  Result<void,ECookError> CookDirectory(std::filesystem::path); // walk tree, refresh .hmeta, cook to .hcooked
  Result<void,ECookError> BuildPak(std::span<const PakInput>, IFile& out);
};
```

- **`ImageCooker`** — reuse the existing stb path (move `stbi_load_from_memory` out of
  `ResourceManager.cpp`): decode → RGBA8 → `HAsset{format=RGBA8_UNORM, extra=HTextureExtra}`. BCn +
  mips are Phase 5 (bc7enc + `stb_image_resize2`).
- **`ShaderCooker`** — reuse `Graphics::ShaderCompiler`; compile per requested `EShaderBackend`
  (WGSL now) and pack into `.hshader`. In the editor it borrows the `ShaderCompiler` already on
  `ENGINE_MANAGER`; the CLI owns one.
  - **Thread-safety (from `-ds`):** `ShaderCompiler` owns a `slang::IGlobalSession` + shared cache and
    is **not** concurrency-safe. Serialize shader cooks behind a mutex, or give each worker its own
    compiler. Image cooks may still parallelize on the engine thread pool.

---

## Runtime read path (`Hush::Assets`)

### `PakFileSystem : IFileSystem` (engine-level, not editor)

- Mount like any backend: `vfs.MountFileSystem<PakFileSystem>("res://", "game.hushpak")`.
- `OpenFile(vfsPath, rel, mode)`: `Fnv1a64(rel)` → binary-search directory → `PakFile : IFile`.
  Uncompressed entries return a span into the mmap via the existing `IFile::Read(size_t)` hook
  (`IFile.hpp:126`, "used when reading mmaped files"); compressed entries zstd-decompress lazily into
  an owned buffer. Write → `OperationNotSupported`. `ListPath` = directory prefix scan.
- mmap behind a small wrapper (`MapViewOfFile` / `mmap`) **plus a full-read fallback** — Emscripten
  has no mmap, so the format must be readable without it (bundle is `--embed-file`'d into the wasm FS).

### Cooked-first resolution (adopted from `-ds`)

Prefer a transparent proxy over sniffing magic bytes in `LoadTexture`. When the engine requests
`res://textures/stone.png`, resolution checks the cooked output first, then falls back to the raw
source. Implement as either a higher-priority mount or a proxy `IFileSystem` that maps a source vpath
to its cooked counterpart (note the extension change `stone.png` → cooked blob key). Consumers
(`ResourceManager::LoadTexture`, `RenderingSystem`) stay unchanged except for handling the cooked
header when present.

---

## Standalone flow (CLI + CMake)

**CLI** (`hush-cooker`, operates on real OS paths via `std::filesystem`, off-VFS):
- `cook --content-dir <dir> --out <bundle> [--compression zstd|none]` — walk tree, create/refresh each
  `.hmeta`, cook to `.hcooked/`, pack into one bundle. Skip-if-fresh via `sourceHash` in the `.hmeta`.
- `cook-file <in> --meta <hmeta> --out <blob>` — single file (editor/debug path).
- `pack <hcooked-dir> --out <bundle>` / `inspect <bundle|blob>` (debug dumps).

**CMake function** in `src/cmake/utils.cmake`, mirroring `enable_reflection` (`:182`) +
`GenerateHushBindings` + example1's post-build copy, with `-ds`'s Emscripten branch:

```cmake
# hush_add_resources(TARGET <t> CONTENT_DIR <dir> OUTPUT <name.hushpak> [COMPRESSION zstd|none])
function(hush_add_resources)
  cmake_parse_arguments(RES "" "TARGET;CONTENT_DIR;OUTPUT;COMPRESSION" "" ${ARGN})
  file(GLOB_RECURSE RES_SRC CONFIGURE_DEPENDS
       ${RES_CONTENT_DIR}/*.png ${RES_CONTENT_DIR}/*.jpg ${RES_CONTENT_DIR}/*.slang ${RES_CONTENT_DIR}/*.hmeta)
  set(_bundle "$<TARGET_FILE_DIR:${RES_TARGET}>/${RES_OUTPUT}")
  add_custom_command(OUTPUT ${_bundle}
    COMMAND $<TARGET_FILE:HushCookerCli> cook --content-dir ${RES_CONTENT_DIR} --out ${_bundle}
            --compression ${RES_COMPRESSION}
    DEPENDS ${RES_SRC} HushCookerCli VERBATIM
    COMMENT "Hush Cooker: cooking resources for ${RES_TARGET}")
  add_custom_target(${RES_TARGET}_resources DEPENDS ${_bundle})
  add_dependencies(${RES_TARGET} ${RES_TARGET}_resources)
  if(EMSCRIPTEN)
    target_link_options(${RES_TARGET} PRIVATE "--embed-file" "${_bundle}@${RES_OUTPUT}")
  endif()   # non-Emscripten: bundle is already written next to the target (no copy needed)
endfunction()
```

Cross-compile caveat: wasm builds need a **host-built** `hush-cooker` (the native tool can't run under
Emscripten) — cook on host, embed the produced bundle.

---

## Editor embedded flow

- **Service**: attach an `AssetCooker`-backed `CookerService` (+ `CookedDirectory`) as a
  component/system on the `ENGINE_MANAGER` singleton entity in `EditorApp::Init`
  (`editor/src/EditorApp.cpp:82-95`), same pattern as `ResourceManager` / `ShaderCompiler`. It borrows
  the sibling `ShaderCompiler`, the VFS, and the project root (`HUSH_DEFAULT_PROJECT_DIR`).
- **OS drop**: add `case SDL_EVENT_DROP_FILE:` to the event switch in
  `rendering/src/WindowRenderer.cpp` (`HandleEvents`, ~`:130`), forwarding `event.drop.data` into an
  editor import queue → copy the file under `res://`, then run the pipeline. (None exists today.)
- **Pipeline** (drop / new source): write `{name}.{ext}.hmeta` (reuse `MakeMetaFile`'s open-Write +
  `SerializeJson`, `ContentPanel.cpp:160-182`, swapping `FileMetadata`→`HMeta`) → `CookToBlob` → write
  `res://.hcooked/{id}.hasset` (or `.hshader`). Emit a `ToastNotification`
  (`CommandPanel.cpp:171`) on success/failure via the existing `NotificationPanel`.
- **File watcher** (`thomasmonkman-filewatch`): watch the real content root; push events to a
  thread-safe queue drained on the editor tick. `OnCreated`/`OnModified` source → refresh `.hmeta` +
  re-cook; `.hmeta` modified → re-cook; `.hmeta` deleted → delete the matching `.hcooked/{id}` output.
  Debounce ~200 ms for save-storms.
  - **Feedback-loop guard (mine; `-ds` omitted this):** the watcher must **ignore events under
    `.hcooked/` and the cooker's own `.hmeta` writes** (path-prefix filter + a short self-write
    suppression set) or it will recook its own outputs endlessly.
- **`CookedDirectory` lifecycle** (from `-ds`): on startup, scan `.hmeta` files, verify/repair
  `.hcooked/` outputs, cook any missing; track `source → {hmeta, cooked}`; GC orphaned cooked outputs
  when a source is deleted.
- **mkdir/delete gap**: `IFileSystem` has no create-dir/delete and doesn't expose the OS root.
  Create/delete `.hcooked` outputs with `std::filesystem::create_directories` / `remove` directly. To
  map a `res://…` vpath to an OS path, add a small helper `VirtualFilesystem::ResolveHostPath(vpath)`
  (or `CFileSystem::GetHostPath`) rather than open-coding `HUSH_DEFAULT_PROJECT_DIR` + remainder.
- **Hide `.hmeta` / `.hcooked`**: filter in `ContentPanel::RefreshDirectory` (`:134`) / `DrawFiles`
  (`:88`) — skip entries whose `extension == HMETA` or whose name is `.hcooked`.
- **Reconcile scaffolding**: move the cooking logic out of `ContentPanel::GenerateMetaFiles /
  CreateInnerResources` into the shared `AssetCooker`; leave `ContentPanel` calling the service + view
  filtering only.

---

## Dependencies to add

| Dependency | Source | Purpose |
|---|---|---|
| `zstd` | vcpkg (`vcpkg.json` + `deps.cmake` `find_package(zstd CONFIG REQUIRED)`) | Per-asset compression; deploy DLL via `hush_deploy_runtime_dlls` |
| `thomasmonkman-filewatch` | vcpkg (header-only) | Editor file watcher |
| `bc7enc` | Vendored `third_party/bc7enc/` (public domain — verify files) | BCn/DXT texture compression (Phase 5) |
| `stb_image_resize2` | Add to `third_party/stb/` | Mipmap generation (Phase 5) |
| `stb_image`, `Slang` | Already present | Decode / shader compile (reused) |

---

## Files to create / modify

**Create**
- `src/engine_core/assets/CMakeLists.txt`, `src/{HAsset,HushPak,HShader,HMeta,AssetFormat,PakFileSystem}.{hpp,cpp}`
- `src/cooker/CMakeLists.txt`, `src/{AssetCooker,ICooker,CookedDirectory}.{hpp,cpp}`,
  `src/Cookers/{ImageCooker,ShaderCooker,ModelCooker}.{hpp,cpp}`, `src/CookerMain.cpp` (CLI),
  editor glue `src/CookerService.{hpp,cpp}`
- `src/editor/src/FileWatcher.{hpp,cpp}` (thin wrapper over the vcpkg filewatch)
- Catch2 tests: `assets/tests/{HAsset,HushPak,HShader,HMeta}.test.cpp`, `cooker/tests/{ImageCooker,ShaderCooker}.test.cpp`
- Docs: `docs/formats/{meta,hasset,hshader}.md`, `docs/diagrams/asset_pipeline.mmd`

**Modify**
- `src/CMakeLists.txt` (`add_subdirectory(cooker)`), `src/engine_core/CMakeLists.txt` (+`assets` in
  `HushLibs`; cooker deliberately excluded), `src/engine_core/cmake/deps.cmake`, `src/cmake/utils.cmake`
  (`hush_add_resources`), `vcpkg.json`
- `src/engine_core/filesystem/src/IFile.hpp` (+ `EFileExtension` values),
  `src/engine_core/filesystem/src/FileSystem.hpp` (`ToKnownExtension`),
  `src/engine_core/filesystem/src/VirtualFilesystem.{hpp,cpp}` (+`ResolveHostPath`, cooked-first mount/proxy)
- `src/engine_core/resources/src/ResourceManager.cpp` (consume cooked texture blob)
- `src/engine_core/rendering/src/WindowRenderer.cpp` (`SDL_EVENT_DROP_FILE`)
- `src/editor/CMakeLists.txt` (link `Hush::Cooker`, add `FileWatcher.cpp`),
  `src/editor/src/EditorApp.cpp` (register `CookerService`/`CookedDirectory`/watcher),
  `src/editor/src/ContentPanel.{hpp,cpp}` (call service + hide `.hmeta`/`.hcooked`),
  `src/editor/src/systems/RenderingSystem.cpp` (load cooked shader)
- `examples/example1/CMakeLists.txt` (`hush_add_resources` + mount the bundle)

---

## Implementation phases (vertical-slice ordered)

Adopts `-ds`'s per-step table format, but **reordered so runtime bundle loading lands with the first
slice** rather than at the end (the standalone mode is useless until a bundle can be read).

### Phase 0 — `Hush::Assets` foundation
| Step | Files | Notes |
|---|---|---|
| 0.1 | `vcpkg.json`, `deps.cmake` | Add `zstd` |
| 0.2 | `assets/CMakeLists.txt`, `engine_core/CMakeLists.txt` | New module; add to `HushLibs` |
| 0.3 | `AssetFormat.hpp`, `HAsset`, `HushPak`, `HShader`, `HMeta` | Structs + codecs; zstd **decompress**; hand-written `HMeta` (de)serialize |
| 0.4 | `PakFileSystem.{hpp,cpp}` | Read-only `IFileSystem` (full-read first; mmap in Phase 4) |
| 0.5 | `assets/tests/` | Round-trip unit tests green |

**Deliverable:** formats + read backend compile and round-trip; no cookers yet.

### Phase 1 — Texture slice, **both modes**, with zstd
| Step | Files | Notes |
|---|---|---|
| 1.1 | `ICooker`, `AssetCooker`, `Cookers/ImageCooker` | stb → RGBA8 `HAsset`; zstd **compress** |
| 1.2 | `CookerMain.cpp`, `cooker/CMakeLists.txt` | CLI `cook`/`cook-file`/`inspect`; `HushCookerCli` exe |
| 1.3 | `utils.cmake`, `examples/example1` | `hush_add_resources`; example mounts bundle, renders cooked texture |
| 1.4 | `ResourceManager.cpp`, VFS cooked-first | Consume cooked texture blob transparently |
| 1.5 | `CookerService`, `ContentPanel`, `WindowRenderer` | Editor drop → `.hmeta`(`name.png.hmeta`) → `.hcooked`; hide `.hmeta` |

**Deliverable:** a folder of PNGs cooks to a bundle rendered by `example1`; the editor cooks a dropped
PNG the same way. **Both modes proven for textures.**

### Phase 2 — Shader slice, both modes
| Step | Files | Notes |
|---|---|---|
| 2.1 | `Cookers/ShaderCooker`, `HShader` | Slang → WGSL packed into `.hshader`; thread-safe compiles |
| 2.2 | `RenderingSystem.cpp` | Load cooked shader instead of `CompileFromSource` |
| 2.3 | `example1` | Cook + load a shader through the bundle |

### Phase 3 — File watcher + lifecycle
| Step | Files | Notes |
|---|---|---|
| 3.1 | `vcpkg.json`, `FileWatcher.{hpp,cpp}` | `thomasmonkman-filewatch`; debounce; **ignore `.hcooked`/self-writes** |
| 3.2 | `CookedDirectory` | Startup reconcile; recook on change; delete cooked on `.hmeta` removal; GC orphans |
| 3.3 | `EditorApp.cpp` | Wire watcher → cook queue drained on tick |

### Phase 4 — mmap + robustness
| Step | Files | Notes |
|---|---|---|
| 4.1 | `PakFileSystem` | Real `MapViewOfFile`/`mmap` + Emscripten full-read fallback; in-place binary search |

### Phase 5 — BCn textures + mips (fast-follow to the issue's DXT)
| Step | Files | Notes |
|---|---|---|
| 5.1 | `third_party/bc7enc/`, `third_party/stb` | Vendor bc7enc; add `stb_image_resize2` |
| 5.2 | `ImageCooker` | BC1/3/5/7 encode + mip chain, driven by `.hmeta.importSettings` |

### Phase 6 — Meshes (deferred)
| Step | Files | Notes |
|---|---|---|
| 6.1 | resources build, `Cookers/ModelCooker` | Re-enable the commented fastgltf loaders; cook glTF → mesh blob |

---

## Documentation updates

| File | Action |
|---|---|
| `docs/formats/meta.md` | `.hmeta` JSON schema, fields, asset types, import settings |
| `docs/formats/hasset.md` | New: `.hasset`/`.hushpak` binary layout + mmap guidance |
| `docs/formats/hshader.md` | New: `.hshader` multi-backend format |
| `docs/diagrams/asset_pipeline.mmd` | Cooker stages: hmeta gen → cook → `.hcooked` → bundle |
| this doc | Keep current as the design index |

Per the project's doc convention, add Breathe/Doxygen API-reference sections to each page once the
corresponding code exists.

---

## Verification

- **Build**: configure/build `windows-x64-debug` (Ninja + vcpkg); confirm zstd + filewatch resolve.
- **Unit**: `ctest` — HAsset/HushPak/HShader byte round-trip, HMeta JSON round-trip, `PakFileSystem`
  open/list on a synthesized bundle.
- **CLI**: `hush-cooker cook --content-dir <sample> --out game.hushpak` then
  `hush-cooker inspect game.hushpak` lists expected entries/formats.
- **Runtime (standalone)**: run `example1` — a texture drawn from the mounted (zstd) bundle matches the
  pre-cook render; a cooked shader binds and draws.
- **Editor (embedded)**: run `HushEditor`; drop a PNG and a `.slang` → `.hmeta` + `.hcooked` outputs
  appear and `.hmeta` is hidden in the Project panel; edit the source → watcher re-cooks (toast); delete
  the `.hmeta` → cooked output is removed.

---

## Risks & open questions

1. **`ShaderCompiler` thread-safety** — global Slang session; serialize shader cooks or one-per-worker.
2. **Emscripten** — no mmap; bundle must load via `--embed-file`'d FS through the full-read fallback.
3. **glTF loaders disabled** — mesh cooking (Phase 6) requires re-enabling the commented fastgltf path.
4. **Multi-backend shaders** — only WGSL is needed today; SPIR-V/DXIL slots reserved, not populated.
5. **bc7enc licensing** — public domain; verify the exact vendored files.
6. **Shader entry-point detection** — default to Slang `[shader("...")]` attributes, else require
   explicit `.hmeta` config.
7. **Cooked-first VFS** — proxy vs mount-priority; handle the source→cooked extension remap.
8. **Asset identity** — `Fnv1a64(vpath)` keys + per-asset `contentHash` for integrity; no SHA/xxHash added.
9. **Encryption** — header bit reserved, deferred (issue says TBD).

---

## Decisions taken (defaults; flag any to change)

- Read path in engine `Hush::Assets`; heavy write path in top-level `src/cooker/`, **out of `HushLibs`**.
- `HMeta` (de)serialization hand-written (no Windows-only reflection codegen).
- Format carries **both** compressed + uncompressed sizes + per-asset magic/version/content-hash.
- `{name}.{ext}.hmeta` naming (collision-safe).
- LE-canonical byte layout; magic + version now, byte-swap deferred.
- Vertical-slice phasing (runtime loading lands in Phase 1, not last).
- Hand-rolled CLI arg parsing (no new arg-parse dep).
- Multi-backend `.hshader` container designed now; WGSL populated, SPIR-V/DXIL reserved.
- BCn + mips as Phase 5 fast-follow; meshes deferred to Phase 6.
