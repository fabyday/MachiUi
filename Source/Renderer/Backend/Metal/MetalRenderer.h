#pragma once
#include "Renderer/IRenderer.h"
#include <memory>

// Call on the AppKit main thread. Targets are borrowed NSWindow pointers;
// unregister each target before closing or destroying its window.
class MetalRendererImpl final : public IRenderer {
public:
    MetalRendererImpl();
    ~MetalRendererImpl() override;
    void onInit(ServiceProvider *provider) override;
    void enqueueRenderCommand(const RenderCommand &cmd) override;
    void attachScene(uint64_t sceneGraphId, ViewId viewId) override;
    void execute() override;
    bool isReady() const;
    void setTargetWindow(ViewId target, void *nativeWindow);
    void removeTargetWindow(ViewId target);
    void execute(const RenderQueue &queue);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
