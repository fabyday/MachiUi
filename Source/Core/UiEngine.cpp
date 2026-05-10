#include "UiEngine.h"
#include "Core/ServiceRegistry.h"
#include "Core/DefaultTimer.h"
#include "Core/LogManager.h"
#include "Core/TaskScheduler.h"
#include "Core/ServiceInitializer.h"
#include "Core/ServiceProvider.h"
#include "Core/InputManager.h"
#include "Core/SceneManager.h"
#include "Core/ViewManger.h"
#include "Scripting/ScriptManager.h"
#include "Renderer/IRenderer.h" // Assuming IRenderer interface is defined here

void UiEngine::_bootstrapComponent()
{
    // ComponentRegistry에서 등록된 모든 컴포넌트의 팩토리를 실행하여 객체를 생성합니다.
    auto &registry = ServiceRegistry::Instance();

    this->m_serviceProvider = std::make_unique<ServiceProvider>();

    ServiceInitializer::createAllServices(registry, *(this->m_serviceProvider.get()));
}

void UiEngine::_initializeComponents()
{
    ServiceInitializer::initializeAllServices(ServiceRegistry::Instance(), *(this->m_serviceProvider.get()));
}

/**
 *
 */
void UiEngine::_initializePlatformDependantComponent()
{
}

void UiEngine::_initializeIOChannel()
{
    auto inputManager = this->m_serviceProvider->getService<InputManager>();

    auto Code = inputManager->ensureDefaultChannels();
    if (isMachiFailed(Code))
    {
        std::cerr << "Failed to initialize input channels: " << Code.msg << std::endl;
    }
}

UiEngine::UiEngine() : engineInitFlag(false) {}

void UiEngine::Init()
{
    if (engineInitFlag)
    {
        // engine was aready initialized.
        return;
    }
    // 1. 컴포넌트 객체 생성
    this->_bootstrapComponent();
    // 2. 컴포넌트 초기화 (의존성 주입 포함)
    this->_initializeComponents();
    this->_initializeIOChannel();
    this->_initializePlatformDependantComponent();

    // Engine
    this->setupFundamentalServices();
    engineInitFlag = true;
}

void UiEngine::setupFundamentalServices()
{
    this->windowHost = this->m_serviceProvider->getService<IWindowHost>();
    this->timer = this->m_serviceProvider->getService<ITimer>();
    this->sceneManager = this->m_serviceProvider->getService<SceneManager>();
    this->scriptManager = this->m_serviceProvider->getService<ScriptManager>();
    this->viewManager = this->m_serviceProvider->getService<ViewManager>();

    // Assuming IRenderer is also a service
    this->renderer = this->m_serviceProvider->getService<IRenderer>();
}

void UiEngine::finalize()
{
    // do nothing, if Engine was not initialized.
    if (!engineInitFlag)
    {
        return;
    }
}

void UiEngine::_updateLayout()
{
}

void UiEngine::update(double deltaTime)
{
    this->windowHost->update();
    if (this->scriptManager != nullptr)
    {
        this->scriptManager->Update();
    }
    this->_updateLayout();
    if (this->renderer != nullptr)
    {
        this->renderer->execute();
    }
}

RuntimeRoot UiEngine::mountScriptView(const std::string &modulePath)
{
    if (this->viewManager == nullptr || this->sceneManager == nullptr || this->scriptManager == nullptr || this->renderer == nullptr)
    {
        return {};
    }

    RuntimeRoot root;
    root.viewId = this->viewManager->createView();
    if (root.viewId == 0)
    {
        return {};
    }

    root.sceneGraphId = this->sceneManager->createSceneGraph(modulePath);
    if (root.sceneGraphId == 0 || !this->sceneManager->createRoot(root.sceneGraphId))
    {
        return {};
    }

    this->renderer->attachScene(root.sceneGraphId, root.viewId);
    this->scriptManager->ExecuteModule(modulePath, root.sceneGraphId);
    return root;
}

// StandAlone Mode
void UiEngine::Run()
{
    // 실제로는 여기에 윈도우 메시지 루프나 종료 조건이 들어갑니다.

    bool running = true;

    // TODO : remove this code after implementing the actual main loop with proper exit conditions.
    if (defaultRoot.viewId == 0)
    {
        defaultRoot = mountScriptView("assets/TestUI/dist/TestUI.js");
    }

    IWindow *win = this->viewManager ? this->viewManager->getWindowByViewId(defaultRoot.viewId) : nullptr;
    if (win == nullptr)
    {
        win = this->windowHost->requestWindow();
    }
    if (win == nullptr)
    {
        return;
    }
    TaskScheduler *scheduler = this->m_serviceProvider->getService<TaskScheduler>();

    win->show();
    win->setTitle("test");
    win->setBorderless(true);

    while (running)
    {
        // upate timer tick
        this->timer->tick();
        // this->m_serviceProvider->getService<LogManager>()->getLogger()->LogDebug("ticktick");
        this->update(this->timer->getDeltaTime());

        ///
        // _sleep(10);
        // async tasks
        scheduler->processReservedTask();
    }
}

void UiEngine::attachCustomRenderer(IRenderer *renderer)
{
    // Bind the custom renderer to the container. Use a custom deleter as UiEngine does not own the raw pointer.
}

void UiEngine::attachCustomWindowHost(IWindowHost *windowHost)
{
    if (this->windowHost)
    {

        // destroy existing window host if already attached, as we are going to replace it with the new one.
        // throw std::runtime_error("Custom WindowHost is already attached.");
    }

    if (windowHost == nullptr)
    {
        throw std::runtime_error("Cannot attach null WindowHost.");
    }

    this->windowHost = windowHost; // Store for UiEngine's direct use
    // Bind the custom window host to the container. Use a custom deleter.
}

void UiEngine::attachCustomTimer(ITimer *timer)
{
    if (this->timer)
    {
        // destroy existing timer if already attached, as we are going to replace it with the new one.
        // throw std::runtime_error("Custom Timer is already attached.");
    }

    if (timer == nullptr)
    {
        throw std::runtime_error("Cannot attach null Timer.");
    }
    this->timer = timer; // Store for UiEngine's direct use
}

void UiEngine::attachCustomLogger(ILogger *logger)
{
    if (this->logger)
    {
        /* code */
    }

    if (logger == nullptr)
    {
        throw std::runtime_error("Cannot attach null Logger.");
    }
    this->logger = logger; // Store for UiEngine's direct use
}
