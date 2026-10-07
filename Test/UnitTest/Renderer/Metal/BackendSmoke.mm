#include "Renderer/Backend/Metal/MetalRenderer.h"
#define Rect MacOSRect
#import <AppKit/AppKit.h>
#undef Rect
#include <cassert>
int main() {
    @autoreleasepool {
        [NSApplication sharedApplication];
        MetalRendererImpl renderer;
        assert(!renderer.isReady());
        renderer.onInit(nullptr);
        assert(renderer.isReady());
        renderer.onInit(nullptr);
        NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 64, 64)
            styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO];
        [window orderFront:nil];
        renderer.setTargetWindow(1, (__bridge void *)window);
        RenderQueue queue;
        queue.recordCommand(1, Color::White());
        queue.recordCommand(1, Rect{8, 8, 24, 24}, Color{1, 0, 0, 0.5f});
        queue.recordCommand(999, Color::Red());
        queue.recordText(1, Rect{0, 0, 64, 24}, "Metal");
        renderer.execute(queue);
        queue.clearCommands();
        renderer.execute(queue);
        renderer.removeTargetWindow(1);
        renderer.execute(queue);
        [window orderOut:nil];
    }
}
