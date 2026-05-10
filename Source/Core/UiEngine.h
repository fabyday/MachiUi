#pragma once
#include <vector>
#include <memory>
#include <type_traits>
#include "IService.h"
#include "IWindowHost.h"
#include "ITimer.h"
#include "ServiceRegistry.h" // For Container
#include "ServiceProvider.h"
#include "../Common/typedef.h"

class ILogger;
class IRenderer;
class SceneManager;
class ScriptManager;
class ViewManager;

struct RuntimeRoot
{
    ViewId viewId = 0;
    uint64_t sceneGraphId = 0;
};

class UiEngine
{
private:
    bool engineInitFlag = false;

protected:
    // construct All Components Objects,
    void _bootstrapComponent();
    // call onInit for all components, this is where dependency injection happens
    void _initializeComponents();
    // initialized Renderer & platform
    void _initializePlatformDependantComponent();
    // init IO Component
    void _initializeIOChannel();

    void setupFundamentalServices();

    //
    void _updateLayout();

public:
    UiEngine();
    ~UiEngine()
    {
        finalize();
    }

    // 엔진 가동: 부품 조립 및 초기화
    void Init();

    // 메인 루프: 모든 부품의 Update 호출
    void Run();
    void finalize();
    RuntimeRoot mountScriptView(const std::string &modulePath);

    // Internal engine update logic.
    // Do not invoke manually in standalone mode.
    void update(double deltaTime);

       /////// register external services(custom implementations) ///////
    void attachCustomRenderer(IRenderer *renderer);
    void attachCustomWindowHost(IWindowHost *windowHost);
    void attachCustomTimer(ITimer *timer);
    void attachCustomLogger(ILogger *logger);

    template <typename T, std::enable_if_t<std::is_base_of_v<IService, T>, int> = 0>
    T *GetService()
    {
        return m_serviceProvider ? m_serviceProvider->getService<T>() : nullptr;
    }

private:
    std::unique_ptr<ServiceProvider> m_serviceProvider;

    IWindowHost *windowHost = nullptr;
    ITimer *timer = nullptr;
    IRenderer *renderer = nullptr; // For UiEngine's direct use
    ILogger *logger = nullptr;     // For passing to LogManager
    SceneManager *sceneManager = nullptr;
    ScriptManager *scriptManager = nullptr;
    ViewManager *viewManager = nullptr;
    RuntimeRoot defaultRoot;
};
