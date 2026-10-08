---
title: Metal Backend
description: macOS renderer integration, supported features, and validation.
sidebar:
  order: 3
---

The Metal backend provides the basic macOS display path for MachiUI. It is
connected to the current ServiceProvider, ViewManager, and scene interfaces.

## Frame Flow

1. CMake compiles `backend.mm` with Objective-C ARC and links AppKit, Metal, and QuartzCore.
2. The renderer registers as the `IRenderer` service and creates a Metal device, command queue, and shader pipelines.
3. `UiEngine::mountScriptView()` creates a native view and scene, attaches them to the renderer, and runs the JavaScript module.
4. React creates native elements through QuickJS bindings.
5. Each frame, the renderer resolves the scene's window, runs Yoga layout, and records rectangle and text commands.
6. A `CAMetalLayer` provides a drawable; Metal encodes the commands and presents the frame.

AppKit and renderer calls must run on the main thread. Layout coordinates use
content-view points with a top-left origin. Drawable dimensions follow the window's
backing scale, including Retina displays and window resizing.

## Current Support

| Feature | Status |
| --- | --- |
| Service registration and scene-to-window attachment | Implemented |
| Solid rectangles and alpha blending | Implemented |
| Retina drawable scaling and resizing | Implemented |
| Basic single-line text | Implemented using an 18-point system font |
| Image textures | Not implemented |
| Clip commands and layer commands | Not implemented |
| Font, color, alignment, and wrapping styles for text | Not implemented |
| macOS pointer and keyboard forwarding to the engine | Not implemented |
| Full native-view adapter synchronization | Not implemented |

Text currently uses bitmap textures and Yoga's approximate text measurement.
The example renders, but this backend is not yet a complete replacement for the
Windows backend. Text bitmap generation also runs each frame; caching and
performance profiling remain future work.

## Run the Example

After building JavaScript and the native targets as described in
[Installation](../../getting-started/installation/):

```sh
cd build-metal
./test2
```

For a bounded run with Metal API validation:

```sh
MTL_DEBUG_LAYER=1 ./test2 --frames 120
MTL_DEBUG_LAYER=1 ./MetalBackendSmoke
```

The integration has been checked by displaying the TestUI dashboard, running
Metal API validation, and passing the unit and smoke tests. These checks
cover startup and basic rendering; they do not establish feature parity or
production performance.

## Direct Renderer Use

For rendering into a dedicated native window outside the scene path:

```cpp
#include "Renderer/Backend/Metal/MetalRenderer.h"

MetalRendererImpl renderer;
renderer.onInit(nullptr);
renderer.setTargetWindow(1, window->getNativeHandle());

RenderQueue queue;
queue.recordCommand(1, Color::White());
queue.recordCommand(1, Rect{20, 20, 120, 60}, Color::Red());
queue.recordText(1, Rect{20, 90, 240, 24}, "Hello, Metal!");
renderer.execute(queue);

renderer.removeTargetWindow(1); // Before closing the native window.
```

The renderer installs a `CAMetalLayer` on the content view. Use a view dedicated
to rendering. Each execution clears visible targets before drawing; an empty
queue clears the previous frame. Text command strings must remain valid until
execution finishes reading the queue.
