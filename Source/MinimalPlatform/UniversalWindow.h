#ifdef _WIN32
#include "Win/Win32Window.h"
#include "Win/WindowManager.h"
#elif defined(__APPLE__)
    #include "OSX/OSXWindow.h"
    // macOS/iOS 전용 코드
#else // LINUX or Unix
#endif