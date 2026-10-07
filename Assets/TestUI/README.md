# TestUI on macOS

Run these commands from the MachiUi directory:

```sh
pnpm --dir Source/Javascript install --frozen-lockfile
pnpm --dir Assets/TestUI install --frozen-lockfile
pnpm --dir Assets/TestUI build
cmake -S . -B build-metal -DBUILD_TEST=OFF
cmake --build build-metal --target test2 -j 4
cd build-metal
./test2
```

The example loads `assets/TestUI/dist/TestUI.js` using QuickJS, creates the React
native tree, lays it out with Yoga, and renders it with Metal. The window displays
the current TestUI demo. Close the window to exit. Run `./test2 --frames 120` for a bounded
smoke run or `MTL_DEBUG_LAYER=1 ./test2 --frames 120` for Metal API validation.

After changing `src/main.tsx`, rebuild the JavaScript bundle and restart `test2`.
Assets are linked into the build directory; run the executable from that directory.
The current display path supports initial mounting, basic width/height styles,
solid backgrounds, and single-line text. React timers, full update/removal support,
images, wrapping, and advanced styling remain unfinished.
