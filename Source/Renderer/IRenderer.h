#pragma once
#include "../Core/IService.h"
#include "RenderQueue.h"

class IRenderer : public IService
{

public:
  RenderQueue renderQueue;
  virtual ~IRenderer() = default;
  virtual void onInit(ServiceProvider *engine) = 0;

  /**
   * ScriptManager call this method to push render command to Renderer,
   * and Renderer will execute these commands in its own timing.
   */
  virtual void enqueueRenderCommand(const RenderCommand &cmd) = 0;

  virtual void execute() = 0;
};