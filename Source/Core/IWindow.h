#pragma once
#include <string>
class IWindow
{
public:
    using NativeHandle = void *;

    virtual ~IWindow() = default;
    // --- 생명 주기 및 제어 ---
    virtual bool init(const std::string &title, uint32_t width, uint32_t height) = 0;
    virtual void update() = 0; // OS 메시지 처리 (PollEvents)
    virtual void close() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual bool shouldClose() const = 0;

    virtual void setBorderless(bool use) = 0;
    virtual void setTitle(const std::string &title) = 0;

    // // --- 창 상태 정보 ---
    // virtual uint32_t GetWidth() const = 0;
    // virtual uint32_t GetHeight() const = 0;
    virtual NativeHandle getNativeHandle() const = 0; // HWND(Win32) 또는 NSWindow*(Mac)

    // // --- 그래픽스 백엔드 연결용 ---
    // // 창 크기 조절 시 SwapChain 재생성을 위해 필요
    // virtual void SetResizeCallback(void (*callback)(uint32_t, uint32_t)) = 0;

    // // --- 창 속성 조절 ---
    // virtual void SetTitle(const std::string &title) = 0;
    // virtual void SetVisible(bool visible) = 0;
};