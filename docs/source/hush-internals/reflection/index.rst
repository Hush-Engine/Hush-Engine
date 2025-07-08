C++ reflection
===================

Hush Engine contains a C++ reflection system that allows you to introspect and manipulate C++ (and scripting) objects at runtime.
This is mostly useful for the editor. We encourage you to **not** rely on this system in your game code.

.. contents::
    :local:

Introduction
------------------
.. _Introduction:
.. index:: Introduction

Hush Engine's reflection system is built around two main components: the runtime reflection system and the tooling
to generate the reflection data.
The runtime reflection system consists of a set of classes that allow you to introspect and manipulate C++ objects at runtime.
The tooling is made up with Libtooling and annotations that allow you to generate the reflection data at compile time.

The reflection system **only** exposes the public members of a class, annotated member functions, and free functions.
This means that you cannot use the reflection system to introspect **non** explicitly annotated members of a class.

The tooling also provides a serialization system that allows you to serialize and deserialize C++ objects to and from JSON.
The serialization system is built around the `rapidjson` library.

Annotations
------------------
.. _Annotations:
.. index:: Annotations

Annotations are used to mark classes, member functions, and free functions that should be included in the reflection system.
Annotations use standard C++ attributes syntax to mark the members.

The following annotations are available:
- [[hush::property]]: Marks a member function as a property. It also receives the following optional arguments:
    - setter: The name of the setter function. If not provided and the member is public, a custom setter is generated.
    - getter: The name of the getter function. If not provided and the member is public, a custom getter is generated.
    - description: A description of the property. This is used for documentation purposes.
- [[hush::function]]: Marks a member function as a function. It also receives the following optional arguments:
    - description: A description of the function. This is used for documentation purposes.
    - command: If it is a free function that receives the editor as argument, this will be exposed **as an editor command** (docs TODO).
- [[hush::reflect]]: Marks a class as reflectable. It also receives the following optional arguments:
    - description: A description of the class. This is used for documentation purposes.

