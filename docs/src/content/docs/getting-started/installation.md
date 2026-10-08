---
title: Installation
description: Build the MachiUI JavaScript assets and native C++ engine on Windows or macOS.
sidebar:
  order: 1
---

Run these commands from the repository root. The native example loads the webpack
bundle in `Assets/TestUI/dist/TestUI.js` using QuickJS.

## Prerequisites

- **macOS:** A Metal-capable GPU and Xcode command-line tools. A graphical session is required to run native window tests.
- **Windows:** Windows 10/11 with DirectX 12 and Visual Studio 2022 or newer with MSVC.
- CMake 3.10 or newer and Git with submodule support.
- Node.js and pnpm. The documentation workflow uses Node.js 24.

## Build JavaScript Assets

```sh
git submodule update --init --recursive
pnpm --dir Source/Javascript install --frozen-lockfile
pnpm --dir Source/Javascript build
pnpm --dir Assets/TestUI install --frozen-lockfile
pnpm --dir Assets/TestUI build
```

The TestUI bundle imports the reconciler source directly. The separate
`Source/Javascript/dist` bundle is useful for other integrations.
CMake links the repository's `Assets` directory into the executable directory as
`assets`; run the native example from that directory.

Watch the asset project during development:

```sh
pnpm --dir Assets/TestUI dev
```

Restart the native example after rebuilding the bundle. Hot reload is not available.

## Build and Run on macOS

```sh
cmake -S . -B build-metal -DBUILD_TEST=OFF
cmake --build build-metal --target test2 MetalBackendSmoke -j 4
cd build-metal
./test2
```

Close the window to exit. A bounded run is useful for smoke checks:

```sh
MTL_DEBUG_LAYER=1 ./test2 --frames 120
MTL_DEBUG_LAYER=1 ./MetalBackendSmoke
```

`BUILD_TEST=OFF` skips GoogleTest and its download step. It does not disable the
example or Metal smoke target. See the [Metal backend guide](../../engine/metal-backend/)
for supported features and current limitations.

## Build and Run on Windows

Use a Visual Studio Developer PowerShell:

```powershell
cmake -S . -B build -DBUILD_TEST=ON
cmake --build build --config Debug
cd build/Debug
./test2.exe
```

## Run Unit Tests

From the repository root:

```sh
cmake -S . -B build-tests -DBUILD_TEST=ON
cmake --build build-tests --config Debug
ctest --test-dir build-tests --output-on-failure -C Debug
```

When enabled, unit tests use GoogleTest through CMake FetchContent. If fetching
fails and you have a local GoogleTest checkout, configure with:

```sh
cmake -S . -B build-tests -DBUILD_TEST=ON \
  -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/absolute/path/to/googletest
```

## Manifest and Native Startup

`UiEngine::Init()` checks `manifest.json` in the working directory. Pass a path
to `Init(path)` to use a different manifest, or `Init("")` to bypass it.

```json
{
  "entry": "./dist/main.js"
}
```

A nonempty `entry` is resolved relative to the manifest directory and mounted by
`Run()`. If `entry` is omitted or empty, `Run()` finds the earliest-created live
ViewManager window. It preserves that window's title and size, and creates a
default window only when no live window exists. Invalid JSON and non-string
entry values are rejected.

The `1.Example` target demonstrates native startup with an empty entry:

```sh
cmake --build build-metal --target MachiUiExample1 -j 4
./build-metal/examples/1.Example/MachiUiExample1
```

The `test2` target mounts Assets/TestUI explicitly; the engine no longer loads
that asset automatically for applications without an entry.
