#include "Win32Window.h"



Win32Window::Win32Window() : hwnd(nullptr)
{
}

Win32Window::~Win32Window()
{
    Close();
}

LRESULT CALLBACK Win32Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}
bool Win32Window::Init(const std::string &title, uint32_t width, uint32_t height)
{
    width = width;
    height = height;

    // 1. 창 클래스 등록 (한 번만 하면 되지만, 보통 헬퍼가 관리)
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      L"MachiWinClass", NULL};
    RegisterClassExW(&wc);

    // 2. 창 생성 (this를 마지막 인자로 넘겨 WndProc에서 낚아챕니다)
    hwnd = CreateWindowExW(0, L"MachiWinClass", L"Editor Window",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                           NULL, NULL, GetModuleHandle(NULL), this);

    if (!hwnd)
        return false;

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    return true;
}
void Win32Window::Update()
{
    MSG msg;
    // 이 특정 창(hwnd)에 대한 메시지만 처리합니다.
    while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    // Win32 메시지 처리 코드
}

void Win32Window::Close()
{
    // Win32 창 닫기 코드
}
bool Win32Window::ShouldClose() const
{
    return false; // 닫힘 상태 반환
}