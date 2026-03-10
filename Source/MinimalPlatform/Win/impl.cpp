#include <Windows.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include "../../Core/IWindow.h"
#ifdef ERROR
#undef ERROR
#endif
#ifdef INFO
#undef INFO
#endif
#endif

#include "osdeps.h"

class Win32Window : public IWindow
{
public:
    Win32Window();          // <--- 선언 추가
    virtual ~Win32Window(); // <--- 선언 추가 (IWindow가 가상이므로 가상 소멸자 권장)
    bool Init(const std::string &title, uint32_t width, uint32_t height) override;
    void Update() override;
    void Close() override;
    void Show() override;
    void Hide() override;   
    bool ShouldClose() const override;

private:
    HWND hwnd; // Win32 창 핸들
};

Win32Window::Win32Window() : hwnd(nullptr)
{
}

Win32Window::~Win32Window()
{
    Close();
}

void Win32Window::Show(){
    ShowWindow(hwnd, SW_SHOWDEFAULT);
}
void Win32Window::Hide(){
    ShowWindow(hwnd, SW_HIDE);
}

bool Win32Window::Init(const std::string &title, uint32_t width, uint32_t height)
{
    width = width;
    height = height;
    return true;
}
void Win32Window::Update()
{
}
void Win32Window::Close()
{
}
bool Win32Window::ShouldClose() const
{
    return false;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

IWindow *create_window(const char *title, int width, int height)
{
    IWindow *win = new Win32Window();
    // 1. 창 클래스 등록 (한 번만 하면 되지만, 보통 헬퍼가 관리)
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      L"MachiWinClass", NULL};
    RegisterClassExW(&wc);

    // 2. 창 생성 (this를 마지막 인자로 넘겨 WndProc에서 낚아챕니다)
    HWND hwnd = CreateWindowExW(0, L"MachiWinClass", L"Editor Window",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                NULL, NULL, GetModuleHandle(NULL), win);

    if (!hwnd)
    {
        delete win;
        return nullptr;
    }

    return win;
    // ShowWindow(hwnd, SW_SHOWDEFAULT);
}

// window callbacks
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:{

        IWindow *targetWindow = reinterpret_cast<IWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (targetWindow)
        {
            targetWindow->Close();
        }
        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}