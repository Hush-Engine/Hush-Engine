C++ Reflection
==============

Hush Engine provides a C++ reflection system for runtime type introspection, dynamic object
construction, and serialization. It is powered by compile-time code generation using
Clang-based tooling (hush-reflection).

.. admonition:: Advanced Feature

    The reflection system is primarily used by the editor and engine infrastructure.
    While fully supported, most gameplay code does not need to use it directly. Consider whether
    your use case requires reflection before adopting it.

.. contents::
    :local:
    :depth: 2

Overview
--------
.. _Overview:
.. index:: Overview

The reflection system has two components:

1. **Annotations + code generation (compile-time)**: You annotate your classes with C++ attributes
   (``[[hush::reflect]]``, ``[[hush::property]]``, ``[[hush::function]]``). The hush-reflection tool
   processes these annotations and generates a ``.hushgen.hpp`` file containing reflection metadata,
   serialization, and deserialization code.

2. **Runtime API**: At runtime, you register reflected types with a ``ReflectionDB`` and use
   ``TypeInfo``, ``FieldInfo``, and ``FunctionInfo`` to query and manipulate objects dynamically.

Only **public** members and **explicitly annotated** members are reflected. Unannotated members
are invisible to the reflection system.

Quick Start
-----------
.. _Quick Start:
.. index:: Quick Start

1. Define a struct with ``[[hush::reflect]]`` and ``HUSH_GENERATED_BODY``:

.. code-block:: cpp

    #include <reflection/Type.hpp>
    #include <Hushgen.hpp>

    // Include the generated file if it exists
    #if __has_include("MyStruct.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
    #include "MyStruct.hushgen.hpp"
    #endif

    struct [[hush::reflect]] MyStruct {
        HUSH_GENERATED_BODY
    public:
        [[hush::function]]
        MyStruct() {}

        [[hush::function]]
        explicit MyStruct(int value) : field(value) {}

        [[hush::function]]
        void SetTo(int value) { field = value; }

        [[hush::property]]
        int field{0};

        [[hush::property]]
        float score{0.0f};

        int notReflected{5}; // This field is NOT visible to reflection
    };

2. Register and query at runtime:

.. code-block:: cpp

    Hush::Reflection::ReflectionDB db;
    MyStruct::RegisterReflection(db);

    // Look up type info by name
    const auto *typeInfo = db.GetTypeInfo("MyStruct");

    // Inspect fields
    auto fields = typeInfo->GetFields(); // span of FieldInfo
    // fields[0].GetName() == "field"
    // fields[1].GetName() == "score"

    // Get and set a field value
    MyStruct instance;
    instance.field = 42;

    auto fieldInfo = typeInfo->GetField("field");
    int newValue = 100;
    fieldInfo->get().Set({Hush::Reflection::VariantView(&instance),
                          Hush::Reflection::VariantView(&newValue)});
    // instance.field == 100

    // Call a reflected function
    int arg = 50;
    typeInfo->CallFunction("SetTo", {Hush::Reflection::VariantView(&instance),
                                     Hush::Reflection::VariantView(&arg)});
    // instance.field == 50

Annotations
-----------
.. _Annotations:
.. index:: Annotations

[[hush::reflect]]
^^^^^^^^^^^^^^^^^

Marks a class or struct as reflectable. Requires ``HUSH_GENERATED_BODY`` inside the class body.

.. code-block:: cpp

    struct [[hush::reflect]] MyComponent {
        HUSH_GENERATED_BODY
    public:
        // ...
    };

Optional parameter:

- ``description``: A description of the class for documentation purposes.

[[hush::property]]
^^^^^^^^^^^^^^^^^^

Marks a member variable for reflection. The hush-reflection tool generates getter and setter
lambdas that directly access the field at compile time, regardless of whether the field is
public or private.

.. code-block:: cpp

    [[hush::property]]
    int health{100};

Optional parameters:

- ``description``: A description of the property.

Both public and private fields work with ``[[hush::property]]``. The generated code accesses
the field directly through inline lambdas, so no public getter/setter methods are required
for the reflection system itself:

.. code-block:: cpp

    struct [[hush::reflect]] MyComponent {
        HUSH_GENERATED_BODY
    public:
        [[hush::property]]
        int publicField{0};   // Works directly

    private:
        [[hush::property]]
        uint32_t m_health = 0; // Also works -- generated code accesses it directly
    };

[[hush::function]]
^^^^^^^^^^^^^^^^^^

Marks a member function (including constructors) for reflection.

.. code-block:: cpp

    [[hush::function]]
    void SetTo10() { this->value = 10; }

    [[hush::function]]
    void SetTo(int newValue) { this->value = newValue; }

    // Constructors can also be reflected
    [[hush::function]]
    MyStruct() {}

    [[hush::function]]
    explicit MyStruct(int val) : value(val) {}

Optional parameters:

- ``description``: A description of the function.
- ``command``: For free functions receiving the editor as an argument, this exposes the function
  as an editor command.

HUSH_GENERATED_BODY
^^^^^^^^^^^^^^^^^^^^

This macro must be placed inside every ``[[hush::reflect]]`` class. The hush-reflection tool replaces it
with implementations for:

- ``TypeId()`` -- returns the type's unique identifier.
- ``TypeName()`` -- returns the type's qualified name.
- ``RegisterReflection(ReflectionDB&)`` -- registers all annotated members.
- ``Serialize(Serializer&)`` -- serializes all properties to a given format.
- ``Deserialize(IVisitor*, EFormatDescribingType)`` -- returns a visitor for deserialization.

The .hushgen.hpp File
^^^^^^^^^^^^^^^^^^^^^

The hush-reflection tool generates a ``.hushgen.hpp`` file for each annotated header at build time.
Include it with the standard pattern:

.. code-block:: cpp

    #if __has_include("MyFile.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
    #include "MyFile.hushgen.hpp"
    #endif

These files are generated automatically and should **not** be manually edited.

Runtime API
-----------
.. _Runtime API:
.. index:: Runtime API

ReflectionDB
^^^^^^^^^^^^^

The central registry for all reflected types.

.. code-block:: cpp

    Hush::Reflection::ReflectionDB db;

    // Register a reflected type
    MyStruct::RegisterReflection(db);

    // Look up by name (uses FNV-1a hash internally)
    const auto *info = db.GetTypeInfo("MyStruct");

    // Look up by TypeId
    const auto *info2 = db.GetTypeInfo(Hush::Reflection::GetTypeId<MyStruct>());

The ``ReflectionDB`` is thread-safe (uses ``std::shared_mutex`` internally).

TypeInfo
^^^^^^^^

Metadata for a reflected type. Obtained from ``ReflectionDB::GetTypeInfo()``.

.. code-block:: cpp

    const auto *typeInfo = db.GetTypeInfo("MyStruct");

    typeInfo->GetName();      // "MyStruct"
    typeInfo->GetSize();      // sizeof(MyStruct)
    typeInfo->GetAlignment(); // alignof(MyStruct)
    typeInfo->GetId();        // TypeId (FNV-1a hash)

    // Inspect members
    typeInfo->GetFields();    // std::span<const FieldInfo>
    typeInfo->GetFunctions(); // std::span<const FunctionInfo>
    typeInfo->GetField("field"); // std::optional<std::reference_wrapper<const FieldInfo>>

    // Create instances dynamically
    auto result = typeInfo->CreateInstance({});
    // result.value() is a Variant containing a MyStruct

    // Call functions by name
    MyStruct instance;
    typeInfo->CallFunction("SetTo10", {Hush::Reflection::VariantView(&instance)});

For advanced use cases, ``CreateInPlaceInstance`` constructs into user-provided memory without
heap allocation:

.. code-block:: cpp

    char buffer[sizeof(MyStruct)];
    int arg = 42;
    auto error = typeInfo->CreateInPlaceInstance(
        buffer, sizeof(buffer),
        {Hush::Reflection::VariantView(&arg)});
    // buffer now contains a MyStruct with field == 42

.. warning::

    When using ``CreateInPlaceInstance``, you are responsible for managing the lifetime of the
    constructed object.

FieldInfo
^^^^^^^^^

Metadata and accessors for a reflected field.

.. code-block:: cpp

    auto fieldOpt = typeInfo->GetField("field");
    const auto &fieldInfo = fieldOpt->get();

    fieldInfo.GetName();   // "field"
    fieldInfo.GetTypeId(); // TypeId for int
    fieldInfo.GetOffset(); // byte offset in the struct

    // Get a field value
    MyStruct instance;
    instance.field = 42;
    auto getResult = fieldInfo.Get({Hush::Reflection::VariantView(&instance)});
    int *value = getResult.value().Get<int>().value(); // *value == 42

    // Set a field value
    int newValue = 100;
    fieldInfo.Set({Hush::Reflection::VariantView(&instance),
                   Hush::Reflection::VariantView(&newValue)});
    // instance.field == 100

The ``Get`` and ``Set`` methods take a span of ``VariantView`` arguments. For ``Get``, pass the
instance. For ``Set``, pass the instance and the new value.

FunctionInfo
^^^^^^^^^^^^

Metadata and invocation for a reflected function.

.. code-block:: cpp

    auto functions = typeInfo->GetFunctions();
    // functions[0].GetName() == "SetTo10"
    // functions[0].GetArgsCount() == 1 (the instance pointer)

    // Call a function
    MyStruct instance;
    auto result = typeInfo->CallFunction("SetTo10",
        {Hush::Reflection::VariantView(&instance)});
    // instance.field == 10

    // Call with arguments
    int value = 20;
    typeInfo->CallFunction("SetTo",
        {Hush::Reflection::VariantView(&instance),
         Hush::Reflection::VariantView(&value)});
    // instance.field == 20

``IsCallableWith`` checks if a function can be called with the given argument types:

.. code-block:: cpp

    bool callable = functions[0].IsCallableWith(
        {Hush::Reflection::VariantView(&instance)});

Variant and VariantView
^^^^^^^^^^^^^^^^^^^^^^^

``Variant`` is a type-erased value container. It uses small-object optimization for types
up to 16 bytes (stored inline) and heap allocation for larger types.

.. code-block:: cpp

    // Create a Variant
    auto variant = Hush::Reflection::Variant::CreateInPlace<int>(42);

    // Check type
    variant.IsType<int>(); // true

    // Extract value
    int *val = variant.Get<int>().value(); // *val == 42

``VariantView`` is a non-owning reference to any typed value. Used throughout the reflection
API to pass arguments:

.. code-block:: cpp

    MyStruct instance;
    Hush::Reflection::VariantView view(&instance);

    // Check type
    view.GetTypeId(); // TypeId for MyStruct

    // Extract
    MyStruct *ptr = view.Get<MyStruct>().value();

TypeId
^^^^^^

A 64-bit identifier for types, computed as the FNV-1a hash of the type name. Use
``GetTypeId<T>()`` to obtain it:

.. code-block:: cpp

    auto id = Hush::Reflection::GetTypeId<int>();
    auto id2 = Hush::Reflection::GetTypeId<MyStruct>();

Built-in specializations exist for: ``int8_t`` through ``int64_t``, ``uint8_t`` through
``uint64_t``, ``float``, ``double``, ``bool``, ``void``, and ``std::string_view``.

Serialization
-------------
.. _Serialization:
.. index:: Serialization

Enabling reflection on a class automatically allows it to be serialized and deserialized.
The generated ``Serialize`` and ``Deserialize`` methods handle all ``[[hush::property]]`` fields.

Here is an example of serializing a reflected struct to JSON:

.. code-block:: cpp

    #include <serialization/Serialization.hpp>

    struct [[hush::reflect]] GameState {
        HUSH_GENERATED_BODY
    public:
        GameState() = default;

        [[hush::property]]
        uint32_t score = 0;

        [[hush::property]]
        uint32_t level = 1;
    };

    // Serialize to JSON
    GameState state;
    state.score = 500;
    state.level = 3;

    auto result = Hush::Serialization::SerializeJson(state);
    // result.value() == {"__type":"GameState","score":500,"level":3}

And deserializing back:

.. code-block:: cpp

    #include <serialization/Deserialization.hpp>

    constexpr std::string_view json = R"({"score": 500, "level": 3})";

    auto result = Hush::Serialization::DeserializeJson<GameState>(json);
    GameState loaded = result.value();
    // loaded.score == 500, loaded.level == 3

The serialization system is built on the ``rapidjson`` library. Full serialization documentation
will be covered in a dedicated section.

Real-World Examples
-------------------
.. _Real-World Examples:
.. index:: Real-World Examples

Transform Component
^^^^^^^^^^^^^^^^^^^

The engine's ``Transform`` component uses both ``[[hush::reflect]]`` and ``[[hush::export]]``
(for scripting bindings):

.. code-block:: cpp

    // From src/engine_core/core/src/Components/Transform.hpp

    struct [[hush::export, hush::reflect]] Transform {
        HUSH_GENERATED_BODY
    public:
        Transform() = default;
        Transform(const glm::vec3 &position,
                  const glm::vec3 &scale = Vector3Math::ONE,
                  const glm::quat &rotation = {});

        [[hush::export]]
        void SetPosition(glm::vec3 position) noexcept;

        // ...
    };

Entity Name
^^^^^^^^^^^

A simple reflected struct used to give entities a display name:

.. code-block:: cpp

    // From src/engine_core/core/src/Entity.hpp

    struct [[hush::reflect]] Name {
        HUSH_GENERATED_BODY
    public:
        std::array<char, MAX_ENTITY_NAME_LENGTH + 1> name{};

        Name() = default;
        Name(const std::string_view &name) { /* ... */ }
    };

API Reference
-------------
.. _API Reference:
.. index:: API Reference

.. doxygenclass:: Hush::Reflection::ReflectionDB
    :members:

.. doxygenclass:: Hush::Reflection::TypeInfo
    :members:

.. doxygenclass:: Hush::Reflection::FieldInfo
    :members:

.. doxygenclass:: Hush::Reflection::FunctionInfo
    :members:

.. doxygenclass:: Hush::Reflection::Variant
    :members:

.. doxygenclass:: Hush::Reflection::VariantView
    :members:
