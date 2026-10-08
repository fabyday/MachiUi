# 1.Example

This example supports manifest-based or native startup.

```sh
cmake -S . -B build -DBUILD_TEST=OFF
cmake --build build --target MachiUiExample1 -j 4
./build/examples/1.Example/MachiUiExample1
```

With the default `EXAMPLE1_MANIFEST_MODE=ON`, CMake copies `manifest.json` into
the example's build directory and passes its path to the executable.
Reconfigure CMake after editing the source manifest.

- A nonempty string `entry` loads that JavaScript module. Relative entry paths
  are resolved against the manifest directory, so place the bundle next to the
  generated manifest at the corresponding path.
- An omitted `entry`, `"entry": ""`, or a missing manifest selects native startup.
- `Run()` uses the earliest-created live ViewManager window. It skips closed or
  destroyed windows, and preserves an existing window's title and size.
- If no live window exists, `Run()` creates and initializes a default window.
- Malformed JSON or a non-string `entry` is rejected.

For native startup, this example creates a 640×480 window titled `1.Example`
before `Run()`. It does not implicitly mount Assets/TestUI.

To bypass the manifest:

```sh
cmake -S . -B build -DBUILD_TEST=OFF -DEXAMPLE1_MANIFEST_MODE=OFF
cmake --build build --target MachiUiExample1 -j 4
./build/examples/1.Example/MachiUiExample1 --frames 120
```

At the engine API level, `Init()` checks `manifest.json` in the working directory,
`Init(path)` uses a specified manifest, and `Init("")` bypasses manifest loading.
The TestUI example uses `mountScriptView()` explicitly.
