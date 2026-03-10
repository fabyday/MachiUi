#import <Cocoa/Cocoa.h>
#include "OSXWindow.h"
// MacWindow.mm 내부 상단에 정의
class OSXWindow : public IWindow
{
public:
    OSXWindow();          // <--- 선언 추가
    virtual ~OSXWindow() override; // <--- 선언 추가 (IWindow가 가상이므로 가상 소멸자 권장)
    bool Init(const std::string &title, uint32_t width, uint32_t height) override;
   void Update() override;
    void Close() override;
    void Show() override;
    void Hide() override;   
    bool ShouldClose() const override;
    
    void setHandle(void* handle);

private:
    void* handle;
};

OSXWindow::OSXWindow() : handle(nullptr) {}
OSXWindow::~OSXWindow() {}


bool OSXWindow::Init(const std::string &title, uint32_t width, uint32_t height)  
{
    return true;
}

void OSXWindow::Update(){
    @autoreleasepool {
        NSEvent* event;
        
        /**
         * untilDate: [NSDate distantPast] 가 핵심입니다.
         * "과거의 시간"을 주면, 지금 당장 쌓여있는 이벤트가 없어도 
         * 스레드를 차단(Block)하지 않고 바로 nil을 반환합니다.
         */
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate distantPast] 
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES])) {
            [NSApp sendEvent:event];
            [NSApp updateWindows];
        }
    }
}
void OSXWindow::Close(){

}
void OSXWindow::Show(){
NSWindow* nsWindow = (__bridge NSWindow*)handle;
    
    // 1. 현재 윈도우의 프레임(위치 + 크기)을 가져옴
    NSRect frame = [nsWindow frame];
    
    // 2. 위치(Origin)는 그대로 두고 크기만 변경
    // macOS 좌표계 특성상 높이가 바뀌면 위쪽으로 늘어나게 하려면 
    // Y축 좌표 보정이 필요할 수 있지만, 기본적으로는 아래와 같이 처리합니다.
    frame.size.width = (CGFloat)600;
    frame.size.height = (CGFloat)800;

    // 3. 변경된 프레임 적용 (animate:YES를 주면 부드럽게 커짐)
    [nsWindow setFrame:frame display:YES animate:NO];
if (handle) {
        NSWindow* nsWindow = (__bridge NSWindow*)handle;
        [nsWindow makeKeyAndOrderFront:nil];
        [nsWindow setIsVisible:YES];          // 2. 가시성 명시
        [NSApp activateIgnoringOtherApps:YES]; // 창을 맨 앞으로 가져오기
    }
}
void OSXWindow::Hide(){

}
bool OSXWindow::ShouldClose() const {
    return false;
}

void OSXWindow::setHandle(void* handle){
    this->handle = handle;
}







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


IWindow* createWindow() {
    // 1. C++ 객체 생성
    OSXWindow* win = new OSXWindow();

    @autoreleasepool {
        // 2. macOS 앱 초기화 (Win32의 RegisterClass 역할)
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        // 3. 윈도우 생성 (CreateWindowExW 대응)
        NSRect frame = NSMakeRect(0, 0, 0, 0);
        NSWindow* nsWindow = [[NSWindow alloc] initWithContentRect:frame
                                                         styleMask:(NSWindowStyleMaskTitled | 
                                                                    NSWindowStyleMaskClosable | 
                                                                    NSWindowStyleMaskResizable)
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO];
        
        [nsWindow setTitle:[NSString stringWithUTF8String:""]];

        // 4. Delegate 연결 (WndProc 연결 작업과 동일)
        MachiWindowDelegate* delegate = [[MachiWindowDelegate alloc] initWithCppWindow:win];
        [nsWindow setDelegate:delegate];

        [nsWindow makeKeyAndOrderFront:nil];
        [NSApp finishLaunching];

        // Win32의 hwnd 보관하듯 NSWindow 포인터를 보관
        win->setHandle((__bridge_retained void*)nsWindow);
        }

    return win;
}