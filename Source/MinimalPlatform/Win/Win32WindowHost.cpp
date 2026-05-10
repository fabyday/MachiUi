#include "Win32WindowHost.h"
#include "../../Core/ServiceRegistry.h"
#include "osdeps.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

Win32WindowHost::Win32WindowHost()
{
}

Win32WindowHost::~Win32WindowHost()
{
}

// IService interface implementation
void Win32WindowHost::onInit(ServiceProvider *provider)
{
    this->inputManager = provider->getService<InputManager>();
}

// For Standalone Mode
IWindow *Win32WindowHost::requestWindow()
{
    IWindow *result = nullptr;
    if (winPool.empty())
    {
        result = createWindow();
        if (result == nullptr)
        {
            return result;
        }
        result->setInputManager(inputManager);
        winPool.push_back(result);
    }
    return result;
}

/**
 * see impl.cpp WndProc
 */
void Win32WindowHost::update()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

REGISTER_UI_COMPONENT_AS(Win32WindowHost, IWindowHost, ServicePhase::System);
