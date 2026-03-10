#include "UiEngine.h"
#include "Core/ComponentRegistry.h"
#include "Core/DefaultTimer.h"
void UiEngine::_bootstrapComponent()
{
    // ComponentRegistry에서 등록된 모든 컴포넌트의 팩토리를 실행하여 객체를 생성합니다.
    auto &registry = ServiceRegistry::Instance();

    // 1. 등록된 모든 단계(System, Logic, Render)를 순서대로 순회
    ServicePhase phases[] = {
        ServicePhase::System,
        ServicePhase::Logic,
        ServicePhase::Render};

    for (auto phase : phases)
    {
        // 2. 해당 단계의 팩토리(공장)들을 가져옴
        const auto &factories = registry.GetFactories(phase);

        for (const auto &factory : factories)
        {
            // 3. 팩토리를 실행하여 실제 객체 생성 (ScriptManager 등이 여기서 태어남)
            auto component = factory();

            // 5. 관리 목록에 추가
            m_components.push_back(std::move(component));
        }
    }
}

void UiEngine::_initializeComponents()
{
    // 이미 _bootstrapComponent에서 OnInit이 호출되었으므로, 여기서는 추가 초기화가 필요한 컴포넌트가 있다면 처리할 수 있습니다.
    // 현재 구조에서는 별도의 초기화 단계가 필요하지 않을 수 있지만, 확장성을 위해 메서드를 분리해두었습니다.
    for (auto &component : m_components)
    {
        component->onInit(this);
    }
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
    IWindow* win = this->windowHost->requestWindow();
    
    while (running)
    {
        // upate timer tick
        this->timer->tick();
        this->update(this->timer->getDeltaTime());
    }
}