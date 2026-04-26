// #pragma once
// #include "../../Core/IWindow.h"
// #include <Windows.h>
// // 윈도우 환경에서만 매크로 충돌을 방지합니다.

// #ifdef _WIN32
//     #ifndef WIN32_LEAN_AND_MEAN
//         #define WIN32_LEAN_AND_MEAN
//     #endif
//     #include <windows.h>

//     // 윈도우가 가로챈 이름을 상우님께 돌려주는 작업입니다.
//     #ifdef ERROR
//         #undef ERROR
//     #endif
//     #ifdef INFO
//         #undef INFO
//     #endif
//     // 만약 WARN이나 DEBUG도 에러가 난다면 똑같이 #undef 해주세요.
// #endif
// class Win32Window : public IWindow
// {
// public:
//     Win32Window();          // <--- 선언 추가
//     virtual ~Win32Window(); // <--- 선언 추가 (IWindow가 가상이므로 가상 소멸자 권장)
//     bool Init(const std::string &title, uint32_t width, uint32_t height) override;
//     void update() override;
//     void close() override;
//     bool ShouldClose() const override;

// private:
//     HWND hwnd; // Win32 창 핸들
// };