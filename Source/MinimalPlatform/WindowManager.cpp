#include <vector>

#include "WindowManager.h"
#include "Win32Window.h"

// Impl 구조체 정의: 실제 데이터들이 여기에 모입니다.
struct WindowManager::Impl
{
    std::vector<std::unique_ptr<IWindow>> windows;
    bool quitSignaled = false;

    // 플랫폼별 창 생성 로직을 Impl 내부에 숨깁니다.
    IWindow *Create(const std::string &title, int w, int h)
    {
        std::unique_ptr<IWindow> win;
        win = std::make_unique<Win32Window>();
        if (win && win->Init(title, w, h))
        {
            IWindow *ptr = win.get();
            windows.push_back(std::move(win));
            return ptr;
        }
        return nullptr;
    }
};

// 생성자에서 Impl 할당WindowManager::WindowManager() : m_pImpl(std::make_unique<WindowManager::Impl>()) {}
WindowManager::WindowManager() : m_pImpl(std::make_unique<WindowManager::Impl>()) {}

// 소멸자 정의 (이 시점에 Impl의 크기를 알아야 하므로 .cpp에 둡니다)
WindowManager::~WindowManager() = default;

IWindow *WindowManager::createWindow(const std::string &title, int w, int h)
{
    return m_pImpl->Create(title, w, h);
}

void WindowManager::launch()
{

    MSG msg;
    while (!m_pImpl->quitSignaled)
    {
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        Sleep(1);
    }
}

void WindowManager::updateAll()
{
    // for (auto &win : m_pImpl->windows)
    // {
    //     win->Update();
    // }
}

bool WindowManager::Quit() const
{
    // return m_pImpl->quitSignaled;
    return true;
}