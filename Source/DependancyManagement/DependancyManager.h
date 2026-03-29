#pragma once
#include <memory>
#include <typeinfo>

class ServiceRegistry;

class DependancyManager
{
public:
    template <typename T>
    std::shared_ptr<T> resolve()
    {
        return std::static_pointer_cast<T>(resolve_impl(typeid(T)));
    }

private:
    std::shared_ptr<void> resolve_impl(const std::type_info &type);
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    ServiceRegistry &serviceRegistry;
};
