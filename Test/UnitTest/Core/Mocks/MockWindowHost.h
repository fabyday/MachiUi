#pragma once
#include <gmock/gmock.h>
#include <Core/IWindowHost.h>
#include <Core/IWindow.h>

using testing::Return;

class MockWindowHost : public IWindowHost
{
public:
    MOCK_METHOD(void, onInit, (UiEngine * engine), (override));
    
    MockWindowHost()
    {
        // do nothing , just return nullptr
        ON_CALL(*this, requestWindow()).WillByDefault(Return(nullptr));
    }
    
    MOCK_METHOD(IWindow *, requestWindow, (), (override));
};
