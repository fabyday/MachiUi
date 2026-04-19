#pragma once
#include "Core/IService.h"
#include "gmock/gmock.h"
#include <Core/IWindow.h>
#include <Core/IWindowHost.h>
#include <gmock/gmock.h>
#include <map>
#include <typeindex>
using testing::Return;
#include <memory>
#include <vector>
#include "Core/ServiceProvider.h"
// this Helper Class mimic UiEngine Class and help dependancy injection.
class MockUiEngine : public ServiceProvider
{
public:
  MockUiEngine()
  {
    // do nothing , just return nullptr
  }
};
