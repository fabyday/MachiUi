#pragma once
#include <vector>
#include <functional>
#include <memory>
#include <map>

#include <typeindex>

#include "IService.h"

enum class ServicePhase
{
    System,
    Logic,
    Render
};

class ServiceRegistry
{
public:
    using Factory = std::function<std::unique_ptr<IService>()>;
    struct ServiceInfo
    {
        std::type_index type;
        Factory factory;
        ServiceInfo()
            : type(typeid(void)),
              factory(nullptr) {};

        ServiceInfo(std::type_index idx, Factory f)
            : type(idx), factory(std::move(f)) {}
    };

    // Meyer's Singleton: 전역 변수 생성 순서 문제를 방지합니다.
    static ServiceRegistry &Instance()
    {
        static ServiceRegistry instance;
        return instance;
    }

    // 컴포넌트 등록 (매크로에서 호출)
    void Register(ServicePhase phase, std::type_index type, Factory factory)
    {
        m_factories[phase].emplace_back(type, factory);
        m_factories_by_type[phase][type] = factory;
    }

    // 특정 단계의 모든 팩토리 가져오기
    const std::vector<ServiceInfo> &GetFactories(ServicePhase phase)
    {
        return m_factories[phase];
    }

private:
    ServiceRegistry() = default;
    std::map<ServicePhase, std::vector<ServiceInfo>> m_factories;
    std::map<ServicePhase, std::unordered_map<std::type_index, Factory>> m_factories_by_type;
};

#include <iostream>

// --- 등록 매크로 ---
// 이 매크로는 .cpp 파일의 전역 영역에서 사용됩니다.
#define REGISTER_UI_COMPONENT(Type, Phase) \
    static bool registered_##Type = []() { \
        std::cout << "Registering component: " << #Type << std::endl; \
        ServiceRegistry::Instance().Register(Phase, std::type_index(typeid(Type)), []() { \
            return std::make_unique<Type>(); \
        }); \
        std::cout << "end" << std::endl; \
        return true; }();

#define REGISTER_UI_COMPONENT_AS(ImplementedType, ConCreteType, Phase) \
    static bool registered_##ConCreteType = []() { \
        std::cout << "Registering component: " << #ConCreteType << std::endl; \
        ServiceRegistry::Instance().Register(Phase, std::type_index(typeid(ConCreteType)), []() { \
            return std::make_unique<ImplementedType>(); \
        }); \
        std::cout << "end" << std::endl; \
        return true; }();