#pragma once
#include <string>
class IWindow
{
public:
    virtual ~IWindow() = default;
    // --- 생명 주기 및 제어 ---
    virtual bool Init(const std::string &title, uint32_t width, uint32_t height) = 0;
    virtual void Update() = 0; // OS 메시지 처리 (PollEvents)
    virtual void Close() = 0;
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual bool ShouldClose() const = 0;


    virtual void setBorderless(bool use) = 0;
    virtual void setTitle(const std::string& title)=0;

    // // --- 창 상태 정보 ---
    // virtual uint32_t GetWidth() const = 0;
    // virtual uint32_t GetHeight() const = 0;
    // virtual void *GetNativeHandle() const = 0; // HWND(Win32) 또는 NSWindow*(Mac)

    // // --- 그래픽스 백엔드 연결용 ---
    // // 창 크기 조절 시 SwapChain 재생성을 위해 필요
    // virtual void SetResizeCallback(void (*callback)(uint32_t, uint32_t)) = 0;

    // // --- 창 속성 조절 ---
    // virtual void SetTitle(const std::string &title) = 0;
    // virtual void SetVisible(bool visible) = 0;
};