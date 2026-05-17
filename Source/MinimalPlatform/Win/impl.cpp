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
    Win32Window();
    virtual ~Win32Window();

    bool init(const std::string &title, uint32_t width, uint32_t height) override;
    void update() override;
    void close() override;
    void show() override;
    void hide() override;
    bool shouldClose() const override;

    void setBorderless(bool use) override;
    void setTitle(const std::string &title) override;
    NativeHandle getNativeHandle() const override;

    void setHWND(HWND hwnd);
    HWND getHWND();

private:
    HWND hwnd;
    bool closeRequested;
};

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

Win32Window::Win32Window() : hwnd(nullptr), closeRequested(false)
{
}

Win32Window::~Win32Window()
{
    close();
}

bool Win32Window::init(const std::string &title, uint32_t width, uint32_t height)
{
    return true;
}

void Win32Window::update()
{
}

void Win32Window::close()
{
    closeRequested = true;
    if (hwnd != nullptr)
    {
        HWND windowToDestroy = hwnd;
        hwnd = nullptr;
        DestroyWindow(windowToDestroy);
    }
}

void Win32Window::show()
{
    ShowWindow(hwnd, SW_SHOWDEFAULT);
}

void Win32Window::hide()
{
    ShowWindow(hwnd, SW_HIDE);
}

bool Win32Window::shouldClose() const
{
    return closeRequested || hwnd == nullptr;
}

void Win32Window::setBorderless(bool use)
{
}

void Win32Window::setTitle(const std::string &title)
{
    SetWindowText(hwnd, title.c_str());
}

IWindow::NativeHandle Win32Window::getNativeHandle() const
{
    return hwnd;
}

void Win32Window::setHWND(HWND hwnd)
{
    this->hwnd = hwnd;
}

HWND Win32Window::getHWND()
{
    return hwnd;
}

IWindow *createWindow()
{
    auto *win = new Win32Window();

    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      MACHI_WINDOW_CLASS_NAME, NULL};
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, MACHI_WINDOW_CLASS_NAME, nullptr,
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                NULL, NULL, GetModuleHandle(NULL), win);
    win->setHWND(hwnd);

    if (!hwnd)
    {
        delete win;
        return nullptr;
    }

    return win;
}

static bool isKeyDown(int virtualKey)
{
    return (GetKeyState(virtualKey) & 0x8000) != 0;
}

static KeyEvent makeKeyEvent(WPARAM wParam, bool isPressed)
{
    return KeyEvent{
        static_cast<int>(wParam),
        isPressed,
        isKeyDown(VK_MENU),
        isKeyDown(VK_CONTROL),
        isKeyDown(VK_SHIFT),
        isKeyDown(VK_LWIN) || isKeyDown(VK_RWIN)};
}

static int mouseButtonBit(MouseButton button)
{
    switch (button)
    {
    case MouseButton::Left:
        return 1;
    case MouseButton::Right:
        return 2;
    case MouseButton::Middle:
        return 4;
    default:
        return 0;
    }
}

static int mouseButtonsFromWParam(WPARAM wParam, MouseButton button, MouseEvent::Type type)
{
    int buttons = 0;
    if ((wParam & MK_LBUTTON) != 0)
    {
        buttons |= mouseButtonBit(MouseButton::Left);
    }
    if ((wParam & MK_RBUTTON) != 0)
    {
        buttons |= mouseButtonBit(MouseButton::Right);
    }
    if ((wParam & MK_MBUTTON) != 0)
    {
        buttons |= mouseButtonBit(MouseButton::Middle);
    }

    if (type == MouseEvent::Type::Down)
    {
        buttons |= mouseButtonBit(button);
    }
    else if (type == MouseEvent::Type::Up)
    {
        buttons &= ~mouseButtonBit(button);
    }

    return buttons;
}

static MouseEvent makeMouseEvent(LPARAM lParam, WPARAM wParam, MouseButton button, MouseEvent::Type type)
{
    const int buttons = mouseButtonsFromWParam(wParam, button, type);
    return MouseEvent{
        type,
        static_cast<float>(static_cast<short>(LOWORD(lParam))),
        static_cast<float>(static_cast<short>(HIWORD(lParam))),
        button,
        buttons != 0,
        buttons};
}

static void handleWindowEvent(IWindow *targetWindow, HWND, UINT message, WPARAM, LPARAM lParam)
{
    if (targetWindow == nullptr || targetWindow->inputManager == nullptr)
    {
        return;
    }

    switch (message)
    {
    case WM_SIZE:
        targetWindow->inputManager->emitWindowEvent(WindowEvent{
            WindowEvent::Type::Resize,
            static_cast<uint32_t>(LOWORD(lParam)),
            static_cast<uint32_t>(HIWORD(lParam))});
        break;
    case WM_SETFOCUS:
        targetWindow->inputManager->emitWindowEvent(WindowEvent{WindowEvent::Type::FocusGained});
        break;
    case WM_KILLFOCUS:
        targetWindow->inputManager->emitWindowEvent(WindowEvent{WindowEvent::Type::FocusLost});
        break;
    case WM_CLOSE:
        targetWindow->inputManager->emitWindowEvent(WindowEvent{WindowEvent::Type::Close});
        break;
    default:
        break;
    }
}

static void handleInputEvent(IWindow *targetWindow, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (targetWindow == nullptr || targetWindow->inputManager == nullptr)
    {
        return;
    }

    switch (message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        targetWindow->inputManager->emitKeyEvent(makeKeyEvent(wParam, true));
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        targetWindow->inputManager->emitKeyEvent(makeKeyEvent(wParam, false));
        break;
    case WM_MOUSEMOVE:
        targetWindow->inputManager->emitMouseEvent(makeMouseEvent(lParam, wParam, MouseButton::Left, MouseEvent::Type::Move));
        break;
    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        SetFocus(hwnd);
        targetWindow->inputManager->emitMouseEvent(makeMouseEvent(lParam, wParam, MouseButton::Left, MouseEvent::Type::Down));
        break;
    case WM_LBUTTONUP:
        ReleaseCapture();
        targetWindow->inputManager->emitMouseEvent(makeMouseEvent(lParam, wParam, MouseButton::Left, MouseEvent::Type::Up));
        break;
    case WM_MBUTTONDOWN:
        SetCapture(hwnd);
        SetFocus(hwnd);
        targetWindow->inputManager->emitMouseEvent(makeMouseEvent(lParam, wParam, MouseButton::Middle, MouseEvent::Type::Down));
        break;
    case WM_MBUTTONUP:
        ReleaseCapture();
        targetWindow->inputManager->emitMouseEvent(makeMouseEvent(lParam, wParam, MouseButton::Middle, MouseEvent::Type::Up));
        break;
    case WM_RBUTTONDOWN:
        SetCapture(hwnd);
        SetFocus(hwnd);
        targetWindow->inputManager->emitMouseEvent(makeMouseEvent(lParam, wParam, MouseButton::Right, MouseEvent::Type::Down));
        break;
    case WM_RBUTTONUP:
        ReleaseCapture();
        targetWindow->inputManager->emitMouseEvent(makeMouseEvent(lParam, wParam, MouseButton::Right, MouseEvent::Type::Up));
        break;
    case WM_MOUSEWHEEL:
        targetWindow->inputManager->emitMouseWheelEvent(MouseWheelEvent{
            static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA)});
        break;
    default:
        break;
    }
}

static bool translateMessage2IMessage(IWindow *targetWindow, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_CLOSE:
        handleWindowEvent(targetWindow, hwnd, message, wParam, lParam);
        return true;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEWHEEL:
        handleInputEvent(targetWindow, hwnd, message, wParam, lParam);
        return true;
    default:
        return false;
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
        auto *createdWindow = static_cast<IWindow *>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createdWindow));
    }

    IWindow *targetWindow = reinterpret_cast<IWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CLOSE:
        handleWindowEvent(targetWindow, hwnd, message, wParam, lParam);
        if (targetWindow != nullptr)
        {
            targetWindow->close();
        }
        else
        {
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        ValidateRect(hwnd, nullptr);
        return 0;
    default:
        break;
    }

    if (translateMessage2IMessage(targetWindow, hwnd, message, wParam, lParam))
    {
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
