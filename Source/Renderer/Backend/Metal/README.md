# Metal backend

The macOS build compiles `backend.mm` with ARC and registers `MetalRendererImpl`
as a Render-phase component. AppKit operations must run on the main thread.
The backend uses BGRA8 render targets, straight-alpha blending, a top-left
coordinate origin, and content-view points (with Retina drawable scaling).

UiEngine uses ServiceProvider and mountScriptView; Metal attaches each scene
to its ViewManager window and submits scene layouts from execute(). To use the renderer directly, initialize and bind it:

```cpp
#include "Renderer/Backend/Metal/MetalRenderer.h"

MetalRendererImpl renderer;
renderer.onInit(nullptr);
renderer.setTargetWindow(1, window->getNativeHandle());

RenderQueue queue;
queue.recordCommand(1, Color::White()); // fill the window
queue.recordCommand(1, Rect{20, 20, 120, 60}, Color::Red());
renderer.execute(queue);
// Call execute again each frame, including after resizing.
renderer.removeTargetWindow(1); // before closing the window
```

Each registered visible window is cleared to transparent black each execution,
then DrawRect commands are drawn in queue order. Empty queues clear old content.
Unknown targets, non-Color payloads, and non-positive/invalid rectangle bounds
are skipped. The backend installs a CAMetalLayer on the window content view;
use a dedicated rendering view rather than a view with an existing custom layer.

DrawText renders a single line using an 18-point macOS system font and a
premultiplied bitmap texture. SetClip, PushLayer, and textured rectangles remain
unimplemented. Text wrapping, font styling, and exact font-based Yoga measurement
are not implemented; the current layout uses a basic text-size estimate.
See `Assets/TestUI/README.md` for the React asset example.

`MetalBackendSmoke` is a standalone macOS smoke target that initializes shaders,
creates a native window, submits full-window and translucent bounded rectangles,
clears an empty frame, and unregisters its target. It requires a macOS graphical
session and Metal device. Run with `MTL_DEBUG_LAYER=1` for API validation.
