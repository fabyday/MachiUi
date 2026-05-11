---
title: Installation
description: Build the MachiUI JavaScript assets and native C++ engine.
sidebar:
  order: 1
---

MachiUI has two build surfaces:

- The JavaScript asset bundle in `Assets/TestUI`.
- The native C++ engine and example executable built with CMake.

## Prerequisites

- Windows 10 or later with DirectX 12 support.
- Visual Studio 2022 or newer with MSVC.
- CMake 3.10 or newer.
- Git with submodule support.
- Node.js and pnpm.

## Build the Asset Bundle

`Assets/TestUI` is a separate webpack project used by the native example application.

```powershell
cd Assets/TestUI
pnpm install
pnpm build
```

Use the development watcher while editing the UI bundle:

```powershell
cd Assets/TestUI
pnpm dev
```

## Build the React Reconciler Bundle

The runtime reconciler bundle is copied into the native executable asset directory during the CMake build.

```powershell
cd Source/Javascript
pnpm install
pnpm build
cd ../..
```

## Build MachiUI

Initialize vendored dependencies before configuring CMake:

```powershell
git submodule update --init --recursive
```

Then configure and build the native project:

```powershell
cmake -S . -B build -DBUILD_TEST=ON
cmake --build build --config Debug
```

The build produces the `MachiUi` static library, the `test2` example executable, and unit test binaries.

## Run Tests

```powershell
ctest --test-dir build --output-on-failure -C Debug
```

## Run the Example

```powershell
.\build\Debug\test2.exe
```
