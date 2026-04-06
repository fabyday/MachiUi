#pragma once
#include <memory>
#include <typeinfo>
#include <memory>
class ServiceRegistry;
class IResolver
{
public:
    virtual ~IResolver() = default;

protected:
};

/**
 *
 */
class DependencyManager
{
private:
    // forward declaration of pimpl to hide boost::di details from header
    struct Impl;
    std::unique_ptr<Impl> pimpl;

protected:
    std::shared_ptr<void> resolve_impl(size_t type_hash);

public:
    template <typename T>
    std::shared_ptr<T> resolve()
    {
        return std::static_pointer_cast<T>(resolve_impl(typeid(T).hash_code()));
    }

    DependencyManager();
    ~DependencyManager();

    template <typename T>
    void registerType();
};
