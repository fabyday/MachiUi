#pragma once
#include <vector>
#include <functional>
#include <memory>
#include <map>

#include "IComponent.h"

enum class ComponentPhase
{
    System,
    Logic,
    Render
};

class ServiceRegistry
{
public:
    using Factory = std::function<std::unique_ptr<IComponent>()>;

    // Meyer's Singleton: 전역 변수 생성 순서 문제를 방지합니다.
    static ServiceRegistry &Instance()
    {
        static ServiceRegistry instance;
        return instance;
    }

    // 컴포넌트 등록 (매크로에서 호출)
    void Register(ComponentPhase phase, Factory factory)
    {
        m_factories[phase].push_back(factory);
    }

    // 특정 단계의 모든 팩토리 가져오기
    const std::vector<Factory> &GetFactories(ComponentPhase phase)
    {
        return m_factories[phase];
    }

private:
    ServiceRegistry() = default;
    std::map<ComponentPhase, std::vector<Factory>> m_factories;
};



#include <iostream>

// --- 등록 매크로 ---
// 이 매크로는 .cpp 파일의 전역 영역에서 사용됩니다.
#define REGISTER_UI_SERVICE(Type, Phase) \
    static bool registered_##Type = []() { \
        std::cout << "Registering Service : " << #Type << std::endl; \
        ServiceRegistry::Instance().Register(Phase, []() { \
            return std::make_unique<Type>(); \
        }); \
        std::cout << "end" << std::endl; \
        return true; }();