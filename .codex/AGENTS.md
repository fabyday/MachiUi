# Codex Agent Guide

This file is the Codex-specific agent guide for MachiUI. The root `AGENTS.md`
is kept for Codex auto-discovery compatibility.

## Project Structure & Module Organization

MachiUI is a C++17 UI engine with CMake as the primary build system. Core
engine code lives in `Source/`: `Core/` for scene, service, and element
primitives; `Renderer/` for render queue and backends; `MinimalPlatform/` for
OS window hosts; `Scripting/` for QuickJS integration; `Elements/` for UI
elements; and `Javascript/` for the React reconciler bundle. Unit tests are
under `Test/UnitTest/` by module. Runtime and demo assets are in `Assets/`,
with `Assets/TestUI/` containing a separate webpack test UI. Vendored
dependencies are in `ThirdParty/`; avoid editing them unless updating the
dependency itself.

## Build, Test, and Development Commands

- `cmake -S . -B build -DBUILD_TEST=ON`: configure the native build and GoogleTest targets.
- `cmake --build build --config Debug`: build the `MachiUi` library, `test2` example executable, and tests.
- `ctest --test-dir build --output-on-failure`: run discovered GoogleTest cases after building.
- `cd Source/Javascript && pnpm install && pnpm build`: build the reconciler bundle into `Source/Javascript/dist`.
- `cd Source/Javascript && pnpm dev`: watch and rebuild the reconciler during JS development.
- `cd Assets/TestUI && pnpm install && pnpm dev`: run the asset test UI webpack watcher.

## Coding Style & Naming Conventions

Use C++17 and follow the existing brace style: class and function braces on
their own lines in C++ files, with 4-space indentation. Public C++ types use
PascalCase (`SceneGraph`, `ElementFactory`), while methods and variables
generally use lower camel case (`setText`, `sceneGraph`). Test files should end
in `Test.cpp`. TypeScript uses ESM imports, 2-space indentation, and camelCase
constants/functions. No formatter config is currently checked in, so match
nearby code and keep generated `dist/`, `build/`, and `node_modules/` files out
of commits.

## Testing Guidelines

Tests use GoogleTest via `FetchContent` in `Test/CMakeLists.txt` and the helper
in `cmakes/test.cmake`. Add tests near the feature module, then register each
executable in `Test/UnitTest/CMakeLists.txt` with `addTest(TargetName
path/to/Test.cpp MACHIUI_TEST_LIB)`. Prefer focused assertions around public
behavior and add regression tests for scripting, layout, renderer, and platform
changes.

## Commit & Pull Request Guidelines

Recent commits use short imperative summaries such as `add backend connect` or
`add View`; keep the first line concise and action-oriented. Pull requests
should describe the change, list build/test commands run, link issues when
applicable, and include screenshots or logs for renderer/UI-visible changes.
