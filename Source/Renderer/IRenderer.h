#pragma once
#include "../Core/IService.h"
#include "RenderQueue.h"

class IRenderer : public IService {

public:
  virtual void execute(const RenderQueue &queue) = 0;
};