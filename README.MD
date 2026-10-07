# MachiUI

**Build declarative React interfaces on a native C++ graphics engine.**

<p align="left">
  <a href="https://fabyday.github.io/MachiUi/">
    <img src="https://img.shields.io/badge/Docs-Open%20MachiUI%20Site-ff5d01?style=for-the-badge&logo=astro&logoColor=white" alt="Open MachiUI Documentation Site" />
  </a>
</p>

## Key Features

- **Native rendering:** DirectX 12 on Windows and Metal on macOS, without an embedded browser.
- **Declarative UI:** A React reconciler maps components and props to native scene elements through QuickJS.
- **Flexbox layout:** Yoga computes element positions and sizes.
- **Service architecture:** `IService`, `ServiceRegistry`, and `ServiceProvider` manage engine services.

## Architecture

1. **React reconciler:** Creates and updates the native UI tree through `MachiNative` bindings.
2. **Scene graph and layout:** C++ elements store attributes and Yoga computes their geometry.
3. **Platform and renderer:** Native window hosts provide windows; DirectX 12 or Metal draws the attached scenes.

The macOS path is connected from `UiEngine::mountScriptView()` through ViewManager,
React mounting, Yoga layout, and Metal frame submission. The TestUI dashboard has
been built and displayed on macOS. Metal currently supports solid rectangles,
alpha blending, Retina scaling, and basic single-line text. Images, clipping,
font styling, text wrapping, and macOS input forwarding remain unfinished.
See the [Metal backend guide](docs/src/content/docs/engine/metal-backend.md).

## Getting Started

Run the following commands from the repository root.

### Prerequisites

- Windows 10/11 with DirectX 12 and Visual Studio 2022 or newer, or macOS with a Metal-capable GPU and Xcode command-line tools.
- CMake 3.10 or newer and Git.
- Node.js and pnpm for JavaScript bundles. The documentation site uses Node.js 24 in CI.
- Vendored QuickJS, Yoga, and spdlog dependencies.

### 1. Initialize Dependencies and Build JavaScript

```sh
git submodule update --init --recursive
pnpm --dir Source/Javascript install --frozen-lockfile
pnpm --dir Source/Javascript build
pnpm --dir Assets/TestUI install --frozen-lockfile
pnpm --dir Assets/TestUI build
```

`Assets/TestUI` is a separate webpack project. It bundles the reconciler source
and example UI into `Assets/TestUI/dist/TestUI.js`. CMake links `Assets` into the
executable directory as `assets`; run the example from that directory.

To rebuild the asset bundle while editing:

```sh
pnpm --dir Assets/TestUI dev
```

Restart the native example after rebuilding; hot reload is not implemented.
`Assets/main.js` also requires the native `MachiNative` host, so running it with
plain Node.js does not launch the native UI.

### 2. Build and Run on macOS

```sh
cmake -S . -B build-metal -DBUILD_TEST=OFF
cmake --build build-metal --target test2 MetalBackendSmoke -j 4
cd build-metal
./test2
```

Close the window to exit. For a bounded run with Metal API validation:

```sh
MTL_DEBUG_LAYER=1 ./test2 --frames 120
MTL_DEBUG_LAYER=1 ./MetalBackendSmoke
```

`BUILD_TEST=OFF` skips GoogleTest downloads while keeping the native example and
Metal smoke target available.

### 3. Build and Run on Windows

In a Visual Studio Developer PowerShell:

```powershell
cmake -S . -B build -DBUILD_TEST=ON
cmake --build build --config Debug
cd build/Debug
./test2.exe
```

### 4. Run Unit Tests

GoogleTest is fetched during configuration when `BUILD_TEST=ON`:

```sh
cmake -S . -B build-tests -DBUILD_TEST=ON
cmake --build build-tests --config Debug
ctest --test-dir build-tests --output-on-failure -C Debug
```

If you already have a GoogleTest checkout, pass
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/absolute/path/to/googletest` to CMake.

## Integration

Built-in backends register as `IRenderer` services. A custom renderer implements
initialization, command enqueueing, scene attachment, and frame execution.
Provide a compatible `IWindowHost` when integrating with a different native
window system. The custom-service attachment methods in `UiEngine` are still
incomplete; they should not be treated as a finished embedding API.

## Documentation

The English documentation site is maintained in `docs/`.

```sh
pnpm --dir docs install --frozen-lockfile
pnpm --dir docs dev
```

See [docs/README.md](docs/README.md) for build settings and
[Assets/TestUI/README.md](Assets/TestUI/README.md) for the macOS example.

## MIT License

Copyright (c) 2026 fabyday

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
