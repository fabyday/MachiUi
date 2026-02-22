

#include <MinimalPlatform/WindowManager.h>
#include <iostream>
#include <string>
#include <Core/UiEngine.h>
#include <Scripting/ScriptManager.h>
int main()
{
    UiEngine engine;
    engine.Init();
    std::cout << "Initializing UI Engine..." << std::endl;
    auto *script = engine.GetComponent<ScriptManager>();
    if (script)
    {
        const char *test = "require('./test.js');const test = 10*100+0.1; console.log(`${test}`);console.log('test');";
        // script->ExecuteModule(test);
        // 처음으로 찾아야 하는 장소,
        // cd 기반
        // assets 저장소 -> 위치는 고정임 (프로그램 설치 위치)
        // 유저 스페이스 =>
        script->ExecuteModule("assets/main.js");
    }
    // engine.Init();
    // engine.Run();
    WindowManager wm;
    wm.createWindow("Test Window", 800, 600);
    wm.launch();
}
