# Contributing to Hush Engine

Welcome, and thank you for your interest in contributing to **Hush Engine**! 🎉

Hush is a C++20 3D game engine built with Vulkan and SDL2, and every contribution — whether it's fixing a typo, reporting a bug, improving documentation, or implementing a new feature — helps move the project forward. We're glad you're here.

This guide will walk you through everything you need to know to get started.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How to Contribute](#how-to-contribute)
- [Getting Started — Development Setup](#getting-started--development-setup)
  - [Prerequisites](#prerequisites)
  - [Clone the Repository](#clone-the-repository)
  - [Bootstrap vcpkg](#bootstrap-vcpkg)
  - [Build the Devtool](#build-the-devtool)
  - [Configure and Build the Engine](#configure-and-build-the-engine)
- [Project Structure](#project-structure)
- [Branching Strategy](#branching-strategy)
- [Code Style](#code-style)
- [Commit Messages](#commit-messages)
- [Pull Request Process](#pull-request-process)
- [Reporting Bugs](#reporting-bugs)
- [Requesting Features](#requesting-features)
- [Documentation](#documentation)
- [License](#license)

---

## Code of Conduct

We are committed to providing a welcoming and inclusive experience for everyone. All participants in the Hush Engine community are expected to follow our [Code of Conduct](CODE_OF_CONDUCT.md). Please read it before participating, and please report any unacceptable behavior.

In short: be respectful, be constructive, and be kind.

---

## How to Contribute

There are many ways to contribute to Hush Engine:

- **🐛 Report bugs** — Found something broken? [Open a bug report.](#reporting-bugs)
- **💡 Suggest features** — Have an idea? [Open a feature request.](#requesting-features)
- **🔧 Submit a pull request** — Fix a bug, improve performance, add a feature, or clean up code.
- **📖 Improve documentation** — Help us make the docs clearer and more complete.
- **💬 Join discussions** — Share ideas, ask questions, or help others in [GitHub Discussions](https://github.com/Hush-Engine/Hush-Engine/discussions).
- **🧪 Write tests** — Improve test coverage across the engine.

If you're unsure where to start, look for issues labeled [`good first issue`](https://github.com/Hush-Engine/Hush-Engine/labels/good%20first%20issue) or ask in Discussions.

---

## Getting Started — Development Setup

### Prerequisites

Make sure you have the following tools installed:

| Tool | Version / Notes |
|------|----------------|
| **C++ Compiler** | MSVC with C++20 support (recommended) |
| **CMake** | 3.26 or later |
| **Ninja** | Build system used by CMake presets |
| **Git** | Any recent version |
| **LLVM / Clang** | 19 (provides `clang-format` and `clang-tidy`) |
| **Rust toolchain** | Stable (for building the devtool) |
| **Python** | 3.10+ (for building documentation) |

> **Note:** The CI builds on Windows with MSVC + Clang 19 (LLVM). MinGW is not supported.

### Clone the Repository

Clone the repo **with submodules** — this is required to pull in the vcpkg dependency manager:

```bash
git clone --recursive https://github.com/Hush-Engine/Hush-Engine.git
cd Hush-Engine
```

If you've already cloned without `--recursive`, initialize the submodules manually:

```bash
git submodule update --init --recursive
```

### Bootstrap vcpkg

vcpkg is used to manage C/C++ dependencies (Catch2, flecs, fmt, glm, spdlog, volk, Vulkan, SDL2, shader-slang, etc.). Bootstrap it before your first build:

```bash
./vcpkg/bootstrap-vcpkg.bat
```

### Build the Devtool

Hush ships with a Rust-based CLI tool called **hush-devtool** that automates configuration, building, and formatting. Build it from source:

```bash
cd src/devtool
cargo build --release
```

The compiled binary will be at `src/devtool/target/release/hush.exe`. You can copy or symlink it to the repository root as `hush-devtool.exe` for convenience.

### Configure and Build the Engine

Use the devtool with one of the available CMake presets:

```bash
# Configure (from the repository root)
./hush-devtool configure --preset windows-x64-debug

# Build
./hush-devtool build --preset windows-x64-debug
```

For a release build, swap `debug` for `release`:

```bash
./hush-devtool configure --preset windows-x64-release
./hush-devtool build --preset windows-x64-release
```

If everything compiles without errors, you're ready to start contributing!

---

## Project Structure

A quick overview of the repository layout:

```text
Hush-Engine/
├── src/
│   ├── engine_core/     # Core engine — rendering, core systems, input, scripting
│   ├── editor/          # Editor application
│   ├── devtool/         # Rust CLI devtool (hush-devtool)
│   ├── bindings/        # Auto-generated C bindings
│   ├── exts/            # Extensions
│   └── ...
├── docs/                # Documentation (Doxygen + Sphinx)
├── examples/            # Example projects
├── res/                 # Resources (shaders, etc.)
├── vcpkg/               # vcpkg submodule
├── CMakeLists.txt       # Root CMake configuration
├── CMakePresets.json     # CMake presets
└── LICENSE              # MIT License
```

---

## Branching Strategy

We use a two-branch model:

| Branch | Purpose |
|--------|---------|
| `main` | Stable, release-ready code |
| `dev` | Active development and integration |

**To contribute:**

1. **Fork** the repository on GitHub.
2. **Create a feature branch** off `dev`:
   ```bash
   git checkout dev
   git pull origin dev
   git checkout -b feature/my-awesome-feature
   ```
3. Make your changes and commit them.
4. **Push** your branch to your fork.
5. **Open a Pull Request** targeting the `dev` branch.

> ⚠️ **Do not submit PRs directly to `main`.** All contributions go through `dev` first.

---

## Code Style

Consistent code style makes the codebase easier to read and maintain. Please follow these guidelines:

### C++

- Run the formatter before every commit:
  ```bash
  ./hush-devtool format
  ```
  This runs `clang-format` under the hood with the project's configuration.
- CI will check formatting with `--check` and fail if there are unformatted files.
- Follow existing patterns and conventions in the codebase.
- Use C++20 features where appropriate.
- Prefer clarity over cleverness.

### Rust (devtool)

- Run the formatter before every commit:
  ```bash
  cd src/devtool
  cargo fmt
  ```
- CI checks Rust formatting with `cargo fmt --check`.

### General Guidelines

- Keep functions focused and reasonably sized.
- Name variables and functions descriptively.
- Add comments for non-obvious logic — but prefer self-documenting code.
- Ensure your code compiles without warnings on both **MSVC** and **Clang**.

---

## Commit Messages

We encourage clear, descriptive commit messages. We recommend the [Conventional Commits](https://www.conventionalcommits.org/) style:

```text
<type>(<scope>): <short description>

<optional body>

<optional footer>
```

**Common types:**

| Type | Description |
|------|-------------|
| `feat` | A new feature |
| `fix` | A bug fix |
| `docs` | Documentation changes |
| `style` | Code formatting (no logic change) |
| `refactor` | Code restructuring (no feature or fix) |
| `test` | Adding or updating tests |
| `build` | Build system or dependency changes |
| `ci` | CI configuration changes |
| `chore` | Maintenance tasks |

**Examples:**

```text
feat(rendering): add PBR material pipeline
fix(input): resolve SDL2 key repeat event duplication
docs: update building instructions for Windows
refactor(core): simplify entity component storage
```

---

## Pull Request Process

1. **Fill out the PR template** — every PR should include a description, related issues, type of change, and the checklist. The [PR template](.github/PULL_REQUEST_TEMPLATE.md) is loaded automatically when you open a PR.

2. **Ensure CI passes** — all checks must be green before a PR can be merged. This includes:
   - Code formatting (`hush-devtool format --check` and `cargo fmt --check`)
   - Successful compilation on Windows (both debug and release)
   - Devtool tests (`cargo test`)

3. **Keep PRs focused** — one feature or fix per PR makes review faster and history cleaner.

4. **Respond to review feedback** — maintainers may request changes. Please address them or discuss alternatives.

5. **Link related issues** — use `Closes #123` in the PR description to auto-close issues on merge.

6. **Add tests** when applicable — especially for new features or bug fixes.

7. **Update documentation** if your change affects public APIs or user-facing behavior.

---

## Reporting Bugs

Found a bug? Please [open a bug report](https://github.com/Hush-Engine/Hush-Engine/issues/new?template=bug_report.yml) using our issue template.

A good bug report includes:

- A clear, concise description of the problem
- Steps to reproduce the behavior
- Expected vs. actual behavior
- Your environment (OS, GPU, driver version, Hush Engine version/commit)
- Any relevant log output or screenshots

---

## Requesting Features

Have an idea for a new feature? Please [open a feature request](https://github.com/Hush-Engine/Hush-Engine/issues/new?template=feature_request.md) using our issue template.

When suggesting a feature, please consider:

- Is it already proposed in an existing issue?
- Does it belong in the engine core, or could it be an extension?
- What problem does it solve?
- Are there alternative approaches?

---

## Documentation

Hush Engine documentation is built with **Doxygen** (API reference) and **Sphinx** with **Breathe** (user-facing docs), and lives in the `docs/` directory.

### Building the Docs

1. Install Python dependencies:
   ```bash
   cd docs
   pip install -r requirements.txt
   ```

2. Build the documentation:
   ```bash
   ./hush-devtool docs
   ```
   Or manually via Sphinx:
   ```bash
   cd docs
   make html
   ```

3. Open `docs/build/index.html` in your browser to preview.

### Writing Documentation

- **API documentation:** Add Doxygen-style comments to public C++ headers.
- **Guides and tutorials:** Write `.rst` files in `docs/source/`.
- **If your PR changes public APIs**, please update the relevant documentation.

The documentation is published at [hushengine.com](https://hushengine.com/).

---

## License

By contributing to Hush Engine, you agree that your contributions will be licensed under the [MIT License](LICENSE), the same license that covers the project.

---

## Questions?

If you have questions that aren't answered here, feel free to:

- Start a thread in [GitHub Discussions](https://github.com/Hush-Engine/Hush-Engine/discussions)
- Check the [documentation](https://hushengine.com/)

Thank you for helping make Hush Engine better! 🚀