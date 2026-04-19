#include "Renderer/IRenderer.h"
#include "Core/ServiceRegistry.h"

#include "Common/macro.h"

MACHI_UI_STATIC bool initMetal();

/// @brief
class MetalRendererImpl : public IRenderer
{

  //   std::unordered_map<uint32_t, void *> nativeHandleMap;

public:
  virtual void onInit(UiEngine *engine) override;
  /// @brief
  /// @param queue
  virtual void execute(const RenderQueue &queue) override;
};

void MetalRendererImpl::onInit(UiEngine *engine) {}

void MetalRendererImpl::execute(const RenderQueue &queue) {}

REGISTER_UI_SERVICE_AS(MetalRendererImpl, IRenderer, ServicePhase::Render)

bool initMetal();