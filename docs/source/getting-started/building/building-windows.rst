Building on Windows
===================

This document will guide you through the process of building Hush Engine on Windows.

Hush natively supports building on Windows using MSVC. MinGW is not supported.


.. contents::
    :local:

Build requirements
------------------
.. _Build requirements:
.. index:: Build requirements

For building Hush Engine, you will need the following:

- CMake 3.26 or later.
- MSVC with C++20 support.
- Ninja
- Hush devtool (optional, but recommended)

The following dependencies are not required to build Hush Engine, but are required for the development workflow:

- Clang-Format 18 or later.
- Clang-Tidy 18 or later.
- Doxygen 1.8.20 or later.
- Python
  - Sphinx
  - Breathe

The following dependencies are required for building the devtool:

- Rust

Building with the devtool
-------------------------
The easiest way to build Hush Engine is to use the devtool. The devtool is a Rust cli tool that automates the process of building Hush Engine.
It will handle commands like `cmake` and `ninja` for you.

To install the devtool, download the latest release from the release page (in progress).

The first step is to clone the repository:

.. code-block:: bash

    git clone https://github.com/Hush-Engine/Hush-Engine.git

Next, navigate to the repository and run the following command:

.. code-block:: bash

    hush configure -p windows-x64-debug
    # or
    hush configure -p windows-x64-release

This will configure the project for building on Windows x64 in debug or release mode.

Finally, run the following command to build the project:

.. code-block:: bash

    hush build -p windows-x64-debug
    # or
    hush build -p windows-x64-release

Building without the devtool
----------------------------

If you don't want to use the devtool, you can build Hush Engine manually.

The first step is to clone the repository:

.. code-block:: bash

    git clone https://github.com/Hush-Engine/Hush-Engine.git

Next, navigate to the repository and run the following command:

.. code-block:: bash

    mkdir build
    cd build
    cmake -G Ninja --preset windows-x64-debug .. -DCMAKE_BUILD_TYPE=Debug

Finally, run the following command to build the project:

.. code-block:: bash

    ninja
    # or
    cmake --build . --config Debug

Building the devtool
--------------------

To build the devtool, navigate to `src/devtool` and run the following command:

.. code-block:: bash

    cargo build --release

This will build the devtool in release mode.

The devtool will be located at `src/devtool/target/release/hush`.

Building the documentation
--------------------------

To build the documentation, run the following command:

.. code-block:: bash

    hush docs

This will generate the documentation in the `docs/build` directory.

To view the documentation, open `docs/build/index.html` in your browser.

.. note::

    The documentation is built using Sphinx and Breathe. Make sure you have these dependencies installed.
