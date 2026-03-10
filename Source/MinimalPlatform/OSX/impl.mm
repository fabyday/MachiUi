#include <Cococa/Cococa.h>

#include "OSXWindow.h"

// MacWindow.mm 내부 상단에 정의
class OSXWindow : public IWindow
{
public:
    OSXWindow();          // <--- 선언 추가
    virtual ~OSXWindow(); // <--- 선언 추가 (IWindow가 가상이므로 가상 소멸자 권장)
    bool Init(const std::string &title, uint32_t width, uint32_t height) override;
    void Update() override;
    void Close() override;
    bool ShouldClose() const override;

private:
};




@interface MachiWindowDelegate : NSObject <NSWindowDelegate> {
    class OSXWindow* cppWindow; // C++ 객체 포인터 (Win32의 GWLP_USERDATA 역할)
}
- (instancetype)initWithCppWindow:(class OSXWindow*)w;
@end

@implementation MachiWindowDelegate
- (instancetype)initWithCppWindow:(class OSXWindow*)w {
    self = [super init];
    if (self) { cppWindow = w; }
    return self;
}

// Win32의 WM_CLOSE / WM_DESTROY 대응
- (BOOL)windowShouldClose:(NSWindow *)sender {
    // 여기서 C++ 클래스의 멤버 변수를 변경하거나 함수를 호출합니다.
    // cppWindow->NotifyClose(); 
    return YES; 
}

- (void)windowWillClose:(NSNotification *)notification {
    // Win32의 PostQuitMessage(0)와 유사한 처리
}
@end


IWindow* create_window(const char* title, int width, int height) {
    // 1. C++ 객체 생성
    MacWindow* win = new MacWindow();

    @autoreleasepool {
        // 2. macOS 앱 초기화 (Win32의 RegisterClass 역할)
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        // 3. 윈도우 생성 (CreateWindowExW 대응)
        NSRect frame = NSMakeRect(0, 0, width, height);
        NSWindow* nsWindow = [[NSWindow alloc] initWithContentRect:frame
                                                         styleMask:(NSWindowStyleMaskTitled | 
                                                                    NSWindowStyleMaskClosable | 
                                                                    NSWindowStyleMaskResizable)
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO];
        
        [nsWindow setTitle:[NSString stringWithUTF8String:title]];

        // 4. Delegate 연결 (WndProc 연결 작업과 동일)
        MachiWindowDelegate* delegate = [[MachiWindowDelegate alloc] initWithCppWindow:win];
        [nsWindow setDelegate:delegate];

        [nsWindow makeKeyAndOrderFront:nil];
        [NSApp finishLaunching];

        // Win32의 hwnd 보관하듯 NSWindow 포인터를 보관
        win->SetHandle((__bridge_retained void*)nsWindow);
    }

    return win;
}