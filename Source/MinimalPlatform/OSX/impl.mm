#include "OSXWindow.h"
#define Rect MacOSRect
#import <Cocoa/Cocoa.h>
#undef Rect
// MacWindow.mm 내부 상단에 정의
class OSXWindow : public IWindow {
public:
  OSXWindow(); // <--- 선언 추가
  virtual ~OSXWindow()
      override; // <--- 선언 추가 (IWindow가 가상이므로 가상 소멸자 권장)
  bool init(const std::string &title, uint32_t width, uint32_t height) override;
  void update() override;
  void close() override;
  void show() override;
  void hide() override;
  bool shouldClose() const override;

  void setBorderless(bool use) override;
  void setTitle(const std::string &title) override;
  void setHandle(void *handle);
  void *getNativeHandle() const override { return handle; }
  Rect getContentBounds() const override {
    NSWindow *window = (NSWindow *)handle;
    NSSize size = window.contentView.bounds.size;
    return {0, 0, (float)size.width, (float)size.height};
  }

private:
  void *handle;
  bool closed;
  id delegate = nil;
public:
  void setDelegate(id value) { delegate = value; }
};

OSXWindow::OSXWindow() : handle(nullptr), closed(false) {}
OSXWindow::~OSXWindow() { close(); [delegate release]; }

bool OSXWindow::init(const std::string &title, uint32_t width,
                     uint32_t height) {
  NSWindow *window = (NSWindow *)handle;
  [window setTitle:[NSString stringWithUTF8String:title.c_str()]];
  [window setContentSize:NSMakeSize(width, height)];
  return handle != nullptr;
}

void OSXWindow::update() {
  @autoreleasepool {
    NSEvent *event;

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
void OSXWindow::close() {
    if (closed) return;
    closed = true;
    NSWindow *window = (NSWindow *)handle;
    if (window) {
        [window setDelegate:nil];
        [window close];
        [window release];
        handle = nullptr;
    }
}
void OSXWindow::show() {
    NSWindow *window = (NSWindow *)handle;
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}
void OSXWindow::hide() {


}
bool OSXWindow::shouldClose() const { return this->closed; }


void OSXWindow::setBorderless(bool use) {
  NSWindow *nsWindow = (__bridge NSWindow *)handle;

  // 현재 스타일 마스크 가져오기
  NSWindowStyleMask style = [nsWindow styleMask];

  if (use) {
    // 테두리 및 타이틀바 관련 비트 제거, Borderless 비트 추가
    style &= ~(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
               NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable);
    style |= NSWindowStyleMaskBorderless;
  } else {
    // 다시 기본 스타일(테두리 있음)로 복구
    style &= ~NSWindowStyleMaskBorderless;
    style |= (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
              NSWindowStyleMaskResizable);
  }

  // 변경된 스타일 적용
  [nsWindow setStyleMask:style];

  // 스타일 변경 후 윈도우가 가끔 갱신되지 않으므로 First Responder 재설정 등
  // 처리
  [nsWindow makeKeyAndOrderFront:nil];

}

void OSXWindow::setTitle(const std::string &title) {
  NSWindow *nsWindow = (__bridge NSWindow *)handle;
  [nsWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
}

void OSXWindow::setHandle(void *handle) { this->handle = handle; }

@interface MachiWindowDelegate : NSObject <NSWindowDelegate> {
  class OSXWindow *OSXMachiWin; // C++ 객체 포인터 (Win32의 GWLP_USERDATA 역할)
}
- (instancetype)initWithCppWindow:(class OSXWindow *)w;
- (BOOL)windowShouldClose:(NSWindow *)sender;
- (void)windowWillClose:(NSNotification *)notification;
@end

@implementation MachiWindowDelegate
- (instancetype)initWithCppWindow:(class OSXWindow *)w {
  self = [super init];
  if (self) {
    OSXMachiWin = w;
  }
  return self;
}

// Win32의 WM_CLOSE / WM_DESTROY 대응
- (BOOL)windowShouldClose:(NSWindow *)sender {
    OSXMachiWin->close(); // 사용자가 닫기를 누르면 close() 호출
  // 여기서 C++ 클래스의 멤버 변수를 변경하거나 함수를 호출합니다.
  // cppWindow->NotifyClose();
  return NO;
}

- (void)windowWillClose:(NSNotification *)notification {
  // Win32의 PostQuitMessage(0)와 유사한 처리
}
@end

IWindow *createWindow() {
  // 1. C++ 객체 생성
  OSXWindow *win = new OSXWindow();

  @autoreleasepool {
    // 2. macOS 앱 초기화 (Win32의 RegisterClass 역할)
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // 3. 윈도우 생성 (CreateWindowExW 대응)
    NSRect frame = NSMakeRect(0, 0, 0, 0);
    NSWindow *nsWindow =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:(NSWindowStyleMaskTitled |
                                               NSWindowStyleMaskClosable |
                                               NSWindowStyleMaskResizable)
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    [nsWindow setTitle:[NSString stringWithUTF8String:""]];

    // 4. Delegate 연결 (WndProc 연결 작업과 동일)
    MachiWindowDelegate *delegate =
        [[MachiWindowDelegate alloc] initWithCppWindow:win];
    [nsWindow setReleasedWhenClosed:NO];
    [nsWindow setDelegate:delegate];
    win->setDelegate(delegate);

    [nsWindow makeKeyAndOrderFront:nil];
    [NSApp finishLaunching];

    // Win32의 hwnd 보관하듯 NSWindow 포인터를 보관

    win->setHandle((void *)nsWindow);
  }

  return win;
}