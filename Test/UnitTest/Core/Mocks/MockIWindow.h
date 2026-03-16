#pragma once
#include <gmock/gmock.h>
#include <Core/IWindow.h>

using namespace testing;

class MockIWindow : public IWindow
{
public:
    virtual ~MockIWindow() = default;

    // --- 생명 주기 및 제어 ---
    // MOCK_METHOD(반환타입, 함수명, (인자들), (한정자들))
    MOCK_METHOD(bool, Init, (const std::string &title, uint32_t width, uint32_t height), (override));
    MOCK_METHOD(void, Update, (), (override));
    MOCK_METHOD(void, Close, (), (override));
    MOCK_METHOD(void, Show, (), (override));
    MOCK_METHOD(void, Hide, (), (override));

    // const 메서드는 뒤에 (const, override)를 붙여줍니다.
    MOCK_METHOD(bool, ShouldClose, (), (const, override));

    MOCK_METHOD(void, setBorderless, (bool use), (override));
    MOCK_METHOD(void, setTitle, (const std::string &title), (override));
    MockIWindow()
    {
        // do nothing , just return nullptr
        ON_CALL(*this, Init(_, Lt(0), _))
            .WillByDefault(Return(false));

        // height(세 번째 인자)가 0보다 작으면 false 리턴
        ON_CALL(*this, Init(_, _, Lt(0)))
            .WillByDefault(Return(false));

        // 그 외의 정상적인 호출에 대해서는 true 리턴 (기본값)
        ON_CALL(*this, Init(_, _, _))
            .WillByDefault(Return(true));
    }
};
