// Carbon also declares a global Rect; keep its legacy type private here.
#define Rect MacOSRect
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#undef Rect
#include "MetalRenderer.h"
#include "Core/ServiceRegistry.h"
#include "Core/ServiceProvider.h"
#include "Core/SceneManager.h"
#include "Core/ViewManger.h"
#include <simd/simd.h>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <cmath>

namespace {
const char *shaderSource = R"(
#include <metal_stdlib>
using namespace metal;
struct Vertex { float2 position; float4 color; float2 uv; };
struct Out { float4 position [[position]]; float4 color; float2 uv; };
vertex Out ui_vertex(uint index [[vertex_id]], constant Vertex *vertices [[buffer(0)]]) {
    Out o; o.position = float4(vertices[index].position, 0, 1);
    o.color = vertices[index].color; o.uv = vertices[index].uv; return o;
}
fragment float4 ui_fragment(Out in [[stage_in]]) { return in.color; }
fragment float4 ui_text(Out in [[stage_in]], texture2d<float> image [[texture(0)]]) {
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);
    return image.sample(s, in.uv);
}
)";
struct Vertex { simd_float2 position; simd_float4 color; simd_float2 uv; };
void requireMainThread() {
    if (![NSThread isMainThread])
        throw std::logic_error("Metal renderer requires the AppKit main thread");
}
}

struct MetalRendererImpl::Impl {
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> commandQueue = nil;
    id<MTLRenderPipelineState> pipeline = nil;
    id<MTLRenderPipelineState> textPipeline = nil;
    std::unordered_map<ViewId, NSWindow *> targets;
    std::unordered_map<uint64_t, ViewId> scenes;
    SceneManager *sceneManager = nullptr;
    ViewManager *viewManager = nullptr;
};

MetalRendererImpl::MetalRendererImpl() : impl(std::make_unique<Impl>()) {}
MetalRendererImpl::~MetalRendererImpl() = default;
bool MetalRendererImpl::isReady() const { return impl->pipeline != nil && impl->textPipeline != nil; }

void MetalRendererImpl::onInit(ServiceProvider *provider) {
    if (provider) {
        impl->sceneManager = provider->getService<SceneManager>();
        impl->viewManager = provider->getService<ViewManager>();
    }
    requireMainThread();
    if (isReady()) return;
    @autoreleasepool {
        impl->device = MTLCreateSystemDefaultDevice();
        if (!impl->device) throw std::runtime_error("Metal device unavailable");
        impl->commandQueue = [impl->device newCommandQueue];
        NSError *error = nil;
        id<MTLLibrary> library = [impl->device newLibraryWithSource:
            [NSString stringWithUTF8String:shaderSource] options:nil error:&error];
        if (!library || !impl->commandQueue)
            throw std::runtime_error("Failed to initialize Metal shaders/command queue");
        MTLRenderPipelineDescriptor *descriptor = [MTLRenderPipelineDescriptor new];
        descriptor.vertexFunction = [library newFunctionWithName:@"ui_vertex"];
        descriptor.fragmentFunction = [library newFunctionWithName:@"ui_fragment"];
        auto attachment = descriptor.colorAttachments[0];
        attachment.pixelFormat = MTLPixelFormatBGRA8Unorm;
        attachment.blendingEnabled = YES;
        attachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
        attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        impl->pipeline = [impl->device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if (!impl->pipeline) throw std::runtime_error("Failed to create Metal render pipeline");
        descriptor.fragmentFunction = [library newFunctionWithName:@"ui_text"];
        // Bitmap text is premultiplied alpha.
        attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
        impl->textPipeline = [impl->device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if (!impl->textPipeline) throw std::runtime_error("Failed to create Metal text pipeline");
    }
}

void MetalRendererImpl::setTargetWindow(ViewId target, void *nativeWindow) {
    requireMainThread();
    if (!nativeWindow) { removeTargetWindow(target); return; }
    impl->targets[target] = (__bridge NSWindow *)nativeWindow;
}
void MetalRendererImpl::removeTargetWindow(ViewId target) {
    requireMainThread();
    impl->targets.erase(target);
}

void MetalRendererImpl::execute(const RenderQueue &queue) {
    requireMainThread();
    if (!isReady()) throw std::logic_error("Metal renderer has not been initialized");
    @autoreleasepool {
        for (const auto &[target, window] : impl->targets) {
            if (![window isVisible] || [window isMiniaturized]) continue;
            NSView *view = window.contentView;
            const CGFloat width = view.bounds.size.width, height = view.bounds.size.height;
            if (width <= 0 || height <= 0) continue;
            CAMetalLayer *layer;
            if ([view.layer isKindOfClass:[CAMetalLayer class]]) {
                layer = (CAMetalLayer *)view.layer;
            } else {
                layer = [CAMetalLayer layer];
                view.wantsLayer = YES;
                view.layer = layer;
            }
            layer.device = impl->device;
            layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            layer.framebufferOnly = YES;
            layer.frame = view.bounds;
            layer.contentsScale = window.backingScaleFactor;
            layer.drawableSize = CGSizeMake(width * layer.contentsScale, height * layer.contentsScale);

            struct Draw { NSUInteger start; id<MTLTexture> texture; };
            std::vector<Draw> draws;
            std::vector<Vertex> vertices;
            for (const auto &command : queue.GetCommands()) {
                if (command.target != target) continue;
                Rect r = command.hasBounds ? command.bounds : Rect{0, 0, (float)width, (float)height};
                Color color{0, 0, 0, 1};
                id<MTLTexture> texture = nil;
                if (command.type == CommandType::DrawRect) {
                    auto value = std::get_if<Color>(&command.data);
                    if (!value) continue;
                    color = *value;
                } else if (command.type == CommandType::DrawText) {
                    auto text = std::get_if<TextData>(&command.data);
                    if (!text || !text->content) continue;
                    NSString *string = [NSString stringWithUTF8String:text->content];
                    if (!string.length) continue;
                    NSDictionary *attributes = @{NSFontAttributeName: [NSFont systemFontOfSize:18],
                        NSForegroundColorAttributeName: NSColor.blackColor};
                    NSSize size = [string sizeWithAttributes:attributes];
                    r.width = std::min(r.width, (float)ceil(size.width));
                    r.height = std::min(r.height, (float)ceil(size.height));
                    if (r.width <= 0 || r.height <= 0) continue;
                    NSInteger pixelsWide = (NSInteger)ceil(r.width * layer.contentsScale);
                    NSInteger pixelsHigh = (NSInteger)ceil(r.height * layer.contentsScale);
                    NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr
                        pixelsWide:pixelsWide pixelsHigh:pixelsHigh bitsPerSample:8 samplesPerPixel:4
                        hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
                        bytesPerRow:pixelsWide * 4 bitsPerPixel:32];
                    if (!bitmap) continue;
                    memset(bitmap.bitmapData, 0, bitmap.bytesPerRow * pixelsHigh);
                    [NSGraphicsContext saveGraphicsState];
                    NSGraphicsContext.currentContext = [NSGraphicsContext graphicsContextWithBitmapImageRep:bitmap];
                    CGContextScaleCTM(NSGraphicsContext.currentContext.CGContext, layer.contentsScale, layer.contentsScale);
                    [string drawAtPoint:NSMakePoint(0, 0) withAttributes:attributes];
                    [NSGraphicsContext restoreGraphicsState];
                    MTLTextureDescriptor *descriptor = [MTLTextureDescriptor
                        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:pixelsWide
                        height:pixelsHigh mipmapped:NO];
                    descriptor.usage = MTLTextureUsageShaderRead;
                    texture = [impl->device newTextureWithDescriptor:descriptor];
                    if (!texture) continue;
                    [texture replaceRegion:MTLRegionMake2D(0, 0, pixelsWide, pixelsHigh) mipmapLevel:0
                        withBytes:bitmap.bitmapData bytesPerRow:bitmap.bytesPerRow];
                } else continue;
                if (!std::isfinite(r.x) || !std::isfinite(r.y) || !std::isfinite(r.width) ||
                    !std::isfinite(r.height) || r.width <= 0 || r.height <= 0) continue;
                float left = 2 * r.x / width - 1, right = 2 * (r.x + r.width) / width - 1;
                float top = 1 - 2 * r.y / height, bottom = 1 - 2 * (r.y + r.height) / height;
                simd_float4 c = {color.r, color.g, color.b, color.a};
                draws.push_back({vertices.size(), texture});
                vertices.insert(vertices.end(), {{{left, top}, c, {0, 0}}, {{left, bottom}, c, {0, 1}},
                    {{right, bottom}, c, {1, 1}}, {{left, top}, c, {0, 0}},
                    {{right, bottom}, c, {1, 1}}, {{right, top}, c, {1, 0}}});
            }
            id<MTLBuffer> buffer = vertices.empty() ? nil : [impl->device
                newBufferWithBytes:vertices.data() length:vertices.size() * sizeof(Vertex)
                options:MTLResourceStorageModeShared];
            if (!vertices.empty() && !buffer) continue;
            id<CAMetalDrawable> drawable = [layer nextDrawable];
            if (!drawable) continue;
            MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
            pass.colorAttachments[0].texture = drawable.texture;
            pass.colorAttachments[0].loadAction = MTLLoadActionClear;
            pass.colorAttachments[0].storeAction = MTLStoreActionStore;
            pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
            id<MTLCommandBuffer> commands = [impl->commandQueue commandBuffer];
            id<MTLRenderCommandEncoder> encoder = [commands renderCommandEncoderWithDescriptor:pass];
            if (!commands || !encoder) continue;
            [encoder setRenderPipelineState:impl->pipeline];
            if (buffer) {
                [encoder setVertexBuffer:buffer offset:0 atIndex:0];
                for (const auto &draw : draws) {
                    [encoder setRenderPipelineState:draw.texture ? impl->textPipeline : impl->pipeline];
                    if (draw.texture) [encoder setFragmentTexture:draw.texture atIndex:0];
                    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:draw.start vertexCount:6];
                }
            }
            [encoder endEncoding];
            [commands presentDrawable:drawable];
            [commands commit];
        }
    }
}
REGISTER_UI_COMPONENT_AS(MetalRendererImpl, IRenderer, ServicePhase::Render)

namespace {
std::optional<Color> backgroundColor(Element *element) {
    const auto attr = element->getAttribute("backgroundColor");
    if (!attr) return std::nullopt;
    const auto value = std::get_if<std::string>(attr);
    if (!value) return std::nullopt;
    if (*value == "red") return Color::Red();
    if (*value == "white") return Color::White();
    if (*value == "black") return Color{0, 0, 0, 1};
    if (value->size() == 7 && value->front() == '#') {
        try { auto rgb = std::stoul(value->substr(1), nullptr, 16);
            return Color{float((rgb >> 16) & 255)/255, float((rgb >> 8)&255)/255, float(rgb&255)/255, 1};
        } catch (...) {}
    }
    return std::nullopt;
}
void recordElement(Element *element, RenderQueue &queue, ViewId target, float x, float y) {
    if (!element || !element->getVisible()) return;
    auto node = element->getLayoutNode();
    x += YGNodeLayoutGetLeft(node); y += YGNodeLayoutGetTop(node);
    Rect rect{x, y, YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node)};
    if (auto color = backgroundColor(element)) queue.recordCommand(target, rect, *color);
    if (!element->getText().empty()) queue.recordText(target, rect, element->getText().c_str());
    for (auto child : element->getChildren()) recordElement(child, queue, target, x, y);
}
}
void MetalRendererImpl::enqueueRenderCommand(const RenderCommand &cmd) {
    renderQueue.recordCommand(cmd);
}
void MetalRendererImpl::attachScene(uint64_t sceneGraphId, ViewId viewId) {
    impl->scenes[sceneGraphId] = viewId;
}
void MetalRendererImpl::execute() {
    requireMainThread();
    RenderQueue queue;
    if (impl->sceneManager && impl->viewManager) {
        for (const auto &[sceneId, viewId] : impl->scenes) {
            auto window = impl->viewManager->getWindowByViewId(viewId);
            auto graph = impl->sceneManager->getSceneGraph(sceneId);
            if (!window || window->shouldClose() || !graph || !graph->getRoot()) {
                removeTargetWindow(viewId);
                continue;
            }
            setTargetWindow(viewId, window->getNativeHandle());
            queue.recordCommand(viewId, Color::White());
            auto bounds = window->getContentBounds();
            YGNodeCalculateLayout(graph->getRoot()->getLayoutNode(), bounds.width, bounds.height, YGDirectionLTR);
            recordElement(graph->getRoot(), queue, viewId, 0, 0);
        }
    }
    for (const auto &command : renderQueue.GetCommands()) queue.recordCommand(command);
    execute(queue);
    renderQueue.clearCommands();
}
