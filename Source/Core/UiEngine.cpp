#include "UiEngine.h"
#include "Core/ComponentRegistry.h"
#include "Core/DefaultTimer.h"
#include "Core/LogManager.h"
#include "Core/TaskScheduler.h"
#include "Renderer/IRenderer.h" // Assuming IRenderer interface is defined here
void UiEngine::_bootstrapComponent()
{
    // ComponentRegistry에서 등록된 모든 컴포넌트의 팩토리를 실행하여 객체를 생성합니다.
    auto &registry = ServiceRegistry::Instance();
}

void UiEngine::_initializeComponents()
{
    
}

/**
 *
 */
void UiEngine::_initializePlatformDependantComponent()
{
}

void UiEngine::_initializeIOComponent()
{
}

UiEngine::UiEngine() : engineInitFlag(false) {}
UiEngine::~UiEngine() {}

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

    this->setupFundamentalServices();
    engineInitFlag = true;
}

void UiEngine::setupFundamentalServices()
{
    this->windowHost = this->GetService<IWindowHost>();
    this->timer = this->GetService<ITimer>();
    this->renderer = this->GetService<IRenderer>(); // Assuming IRenderer is also a service
}

void UiEngine::finalize()
{
}

void UiEngine::_updateLayout()
{
}

void UiEngine::update(double deltaTime)
{

    this->_updateLayout();
}

// StandAlone Mode
void UiEngine::Run()
{
    // 실제로는 여기에 윈도우 메시지 루프나 종료 조건이 들어갑니다.
    bool running = true;
    IWindow *win = this->windowHost->requestWindow();
    TaskScheduler *scheduler = this->GetService<TaskScheduler>();

    win->Show();
    win->setTitle("test");
    win->setBorderless(true);

    while (running)
    {
        // upate timer tick
        this->timer->tick();

        win->Update();
        // this->GetService<LogManager>()->getLogger()->LogDebug("ticktick");
        this->update(this->timer->getDeltaTime());

        scheduler->processReservedTask();
    }
}

void UiEngine::attachCustomRenderer(IRenderer *renderer)
{
    // Bind the custom renderer to the container. Use a custom deleter as UiEngine does not own the raw pointer.
    m_container.bind<IRenderer>(std::shared_ptr<IRenderer>(renderer, [](IRenderer *) {}));
}

void UiEngine::attachCustomWindowHost(IWindowHost *windowHost)
{
    if (this->windowHost)
    {

        // destroy existing window host if already attached, as we are going to replace it with the new one.
        // throw std::runtime_error("Custom WindowHost is already attached.");
    }

    if (windowhost == nullptr)
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