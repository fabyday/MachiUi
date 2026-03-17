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

#include <vector>
#include <string>
#include <algorithm>

class Container {
    std::unordered_map<std::type_index, std::shared_ptr<void>> services;
    std::vector<std::type_index> resolve_stack; // 순환 참조 추적용 스택

public:
    struct Resolver {
        Container& container;
        template<typename T>
        operator std::shared_ptr<T>() const { return container.resolve<T>(); }
    };

    template<typename T>
    void bind(std::shared_ptr<T> instance) {
        services[typeid(T)] = instance;
    }

    template<typename T>
    std::shared_ptr<T> resolve() {
        auto tidx = std::type_index(typeid(T));

        // 1. 이미 있으면 반환
        if (services.count(tidx)) 
            return std::static_pointer_cast<T>(services[tidx]);

        // 2. 순환 참조 검사
        if (std::find(resolve_stack.begin(), resolve_stack.end(), tidx) != resolve_stack.end()) {
            std::string path;
            for (auto& s : resolve_stack) path += std::string(s.name()) + " -> ";
            throw std::runtime_error("Circular Dependency: " + path + tidx.name());
        }

        // 3. 생성 시도 (재귀 시작)
        resolve_stack.push_back(tidx);
        
        // 주의: 실제 구현 시에는 생성자 인자 개수에 따른 가변 처리가 필요합니다.
        // 여기서는 예시로 인자 2개까지 자동 매칭한다고 가정합니다.
        auto inst = std::make_shared<T>(Resolver{*this}, Resolver{*this}); 
        
        bind<T>(inst);
        resolve_stack.pop_back();
        
        return inst;
    }
};
class ServiceRegistry {
public:
    // 팩토리가 이제 Container를 인자로 받습니다.
    using Factory = std::function<std::shared_ptr<IService>(Container&)>;

    static ServiceRegistry &Instance() {
        static ServiceRegistry instance;
        return instance;
    }

    void Register(ServicePhase phase, std::type_index tidx, Factory factory) {
        m_factories[phase].push_back({tidx, factory});
    }

    struct Entry { std::type_index tidx; Factory factory; };
    const std::vector<Entry>& GetEntries(ServicePhase phase) { return m_factories[phase]; }

private:
    std::map<ServicePhase, std::vector<Entry>> m_factories;
};

// --- 등록 매크로 ---
// 이 매크로는 .cpp 파일의 전역 영역에서 사용됩니다.
#define REGISTER_UI_SERVICE(Type, Phase) \
    static bool registered_##Type = []() { \
        ServiceRegistry::Instance().Register(Phase, typeid(Type), [](Container& c) { \
            return c.resolve<Type>(); \
        }); \
        return true; \
    }();