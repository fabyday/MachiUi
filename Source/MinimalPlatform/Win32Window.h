#pragma once
#include "../Core/IWindow.h"
#include <Windows.h>

class Win32Window : public IWindow
{
public:
    Win32Window();          // <--- 선언 추가
    virtual ~Win32Window(); // <--- 선언 추가 (IWindow가 가상이므로 가상 소멸자 권장)
    bool Init(const std::string &title, uint32_t width, uint32_t height) override;
    void Update() override;
    void Close() override;
    bool ShouldClose() const override;
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    HWND hwnd; // Win32 창 핸들
};