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

#define MACHI_WINDOW_CLASS_NAME L"MachiWinClass"

class Win32Window : public IWindow
{
public:
    Win32Window();          // <--- 선언 추가
    virtual ~Win32Window(); // <--- 선언 추가 (IWindow가 가상이므로 가상 소멸자 권장)
    bool init(const std::string &title, uint32_t width, uint32_t height) override;
    void update() override;
    void close() override;
    void show() override;
    void hide() override;
    bool shouldClose() const override;

    virtual void setBorderless(bool use) override;
    virtual void setTitle(const std::string &title) override;

    virtual NativeHandle getNativeHandle() const override;

    void setHWND(HWND hwnd);
    HWND getHWND();

private:
    HWND hwnd; // Win32 창 핸들
};

void Win32Window::setBorderless(bool use)
{
}

void Win32Window::setTitle(const std::string &title)
{
    SetWindowText(hwnd, title.c_str());
}

Win32Window::Win32Window() : hwnd(nullptr)
{
}

Win32Window::~Win32Window()
{
    close();
}

void Win32Window::show()
{
    ShowWindow(hwnd, SW_SHOWDEFAULT);
}
void Win32Window::hide()
{
    ShowWindow(hwnd, SW_HIDE);
}

bool Win32Window::init(const std::string &title, uint32_t width, uint32_t height)
{
    width = width;
    height = height;
    return true;
}
void Win32Window::setHWND(HWND hwnd)
{
    this->hwnd = hwnd;
}
HWND Win32Window::getHWND()
{
    return hwnd;
}
void Win32Window::update()
{

    MSG msg;
    while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
void Win32Window::close()
{
    DestroyWindow(hwnd);
}
bool Win32Window::shouldClose() const
{

    return false;
}

IWindow::NativeHandle Win32Window::getNativeHandle() const
{
    return hwnd;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

IWindow *createWindow()
{
    Win32Window *win = new Win32Window();

    // 1. 창 클래스 등록 (한 번만 하면 되지만, 보통 헬퍼가 관리)
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      MACHI_WINDOW_CLASS_NAME, NULL};
    RegisterClassExW(&wc);

    // 2. 창 생성 (this를 마지막 인자로 넘겨 WndProc에서 낚아챕니다)
    HWND hwnd = CreateWindowExW(0, MACHI_WINDOW_CLASS_NAME, nullptr,
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, GetModuleHandle(NULL), win);
    win->setHWND(hwnd);

    if (!hwnd)
    {
        delete win;
        return nullptr;
    }

    return win;
    // ShowWindow(hwnd, SW_SHOWDEFAULT);
}

// window callbacks
/**
 * System Input( win32 or Mac )
 */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
    {

        IWindow *targetWindow = reinterpret_cast<IWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (targetWindow)
        {
            targetWindow->close();
        }
        PostQuitMessage(0);
        return 0;
    }
    case WM_CLOSE:
    
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}