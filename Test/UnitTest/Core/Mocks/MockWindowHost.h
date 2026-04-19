#pragma once
#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <Core/IWindowHost.h>
#include <Core/IWindow.h>
#include "MockUiEngine.h"
#include "MockIWindow.h"
#include <Core/ServiceProvider.h>

#include <vector>
#include <memory>
using testing::Return;

class MockWindowHost : public IWindowHost
{
    std::vector<std::unique_ptr<MockIWindow>> windows;

public:
    MOCK_METHOD(void, onInit, (ServiceProvider * provider), (override));

    MockWindowHost()
    {
        // do nothing , just return nullptr
        ON_CALL(*this, requestWindow()).WillByDefault(testing::Invoke([this]()
                                                                      {  
            auto newWin = std::make_unique<MockIWindow>();
            windows.push_back(std::move(newWin));
            return windows.back().get(); }));
    }

    MOCK_METHOD(IWindow *, requestWindow, (), (override));
};
