

#include <MinimalPlatform/WindowManager.h>
#include <iostream>
#include <string>
#include <Core/UiEngine.h>
#include <Scripting/ScriptManager.h>
#include <Core/LogManager.h>
#include <Core/ILogger.h>

int main()
{
    UiEngine engine;
    engine.Init();
    std::cout << "Initializing UI Engine..." << std::endl;
    auto *script = engine.GetComponent<ScriptManager>();

    auto *logMng = engine.GetComponent<LogManager>();
    auto logger = logMng->getLogger();
    logger->setLevel(Level::DEBUG);
    logger->Log(Level::INFO, "test log");
    logger->Log(Level::INFO, "{} {}", "Test", "@");
    logger->Log(Level::DEBUG, "test log");
    if (script)
    {
        const char *test = "console.log;console.log(globalThis.machiRenderer)";
        // script->ExecuteModule(test);
        // 처음으로 찾아야 하는 장소,
        // cd 기반
        // assets 저장소 -> 위치는 고정임 (프로그램 설치 위치)
        // 유저 스페이스 =>
        script->ExecuteModule("assets/TestUI/dist/TestUI.js");
        // script->ExecuteModule("assets/platform/machiUI-reconciler.js");
        // script->ExecuteModule("assets/main.js");
        script->Execute(test);
    }
    // engine.Init();
    // engine.Run();
    WindowManager wm;
    wm.createWindow("Test Window", 800, 600);
    wm.launch();
}
