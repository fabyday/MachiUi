#pragma once
#include <vector>
#include <memory>
#include "IService.h"
#include "IWindowHost.h"
#include "ITimer.h"
#include "ServiceRegistry.h" // For Container
class ILogger;
class IRenderer;

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
    void _initializeIOComponent();

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

    // Internal engine update logic.
    // Do not invoke manually in standalone mode.
    void update(double deltaTime);

       /////// register external services(custom implementations) ///////
    void attachCustomRenderer(IRenderer *renderer);
    void attachCustomWindowHost(IWindowHost *windowHost);
    void attachCustomTimer(ITimer *timer);
    void attachCustomLogger(ILogger *logger);

private:
    std::unique_ptr<ServiceProvider> m_serviceProvider;

    IWindowHost *windowHost = nullptr;
    ITimer *timer = nullptr;
    IRenderer *renderer = nullptr; // For UiEngine's direct use
    ILogger *logger = nullptr;     // For passing to LogManager
};