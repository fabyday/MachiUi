#include "DependancyManager.h"

#include <boost-di/di.hpp>
#include <typeindex>

#include <unordered_map>
#include <memory>
#include <Core/ComponentRegistry.h>

namespace di = boost::di;
// DependencyManager.cpp
struct DependencyManager::Impl
{
    di::injector<> injector;

    // 2. 타입 해시를 키로 하는 생성 함수 지도
    // void*로 리턴하지만 실제로는 shared_ptr<T>가 담김
    using FactoryFunc = std::function<std::shared_ptr<void>(const decltype(injector) &)>;
    std::unordered_map<size_t, FactoryFunc> registries;

    Impl()
    {
        const auto injector = di::make_injector(
            di::bind<int>().to(64), // bind interface to implementation
            di::bind<float>().to(632.0f) // bind interface to implementation
        );
    }

    template <typename T>
    void register_type()
    {
        registries[typeid(T).hash_code()] = [](const auto &i)
        {
            return std::static_pointer_cast<void>(i.template create<std::shared_ptr<T>>());
        };
    }
};

std::shared_ptr<void> DependencyManager::resolve_impl(size_t type_hash)
{
    auto it = pimpl->registries.find(type_hash);
    if (it != pimpl->registries.end())
    {
        // 등록된 람다 실행 -> boost::di가 의존성을 주입하며 객체 생성
        return it->second(pimpl->injector);
    }
    return nullptr; // 혹은 예외 처리
}

DependencyManager::DependencyManager() : pimpl(std::make_unique<Impl>())
{
}

DependencyManager::~DependencyManager() = default;

template <typename T>
void DependencyManager::registerType()
{
    pimpl->register_type<T>();
}

std::shared_ptr<void> DependencyManager::resolve_impl(size_t type_hash)
{
    return pimpl->registries.count(type_hash) ? pimpl->registries.at(type_hash)(pimpl->injector) : nullptr;
}