#include <MachiUi.h>
#include <Scripting/ScriptManager.h>

int main(int argc, char **argv)
{
    UiEngine engine;
#if defined(MANIFEST_FILE_MODE)
    engine.Init(MACHIUI_EXAMPLE_MANIFEST);
    const bool hasEntry = engine.GetService<ScriptManager>()->readManifestEntry(MACHIUI_EXAMPLE_MANIFEST).has_value();
#else
    engine.Init("");
    const bool hasEntry = false;
#endif
    // Configure the native initial window before entering the frame loop.
    if (!hasEntry)
    {
        auto views = engine.GetService<ViewManager>();
        auto window = views->getWindowByViewId(views->createView());
        if (!window || !window->init("1.Example", 640, 480)) return 1;
    }
    uint32_t frames = argc == 3 && std::string(argv[1]) == "--frames" ? std::stoul(argv[2]) : 0;
    engine.Run(frames);
    engine.finalize();
}
