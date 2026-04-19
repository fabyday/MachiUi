#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <utility>

// forward declaration to avoid circular dependency
class IService;

/**
 * ServiceProvider Controls the lifecycle and access to all services in the engine.
 * It allows services to be registered and retrieved by type.
 */
class ServiceInitializer;

class ServiceProvider
{

    //
public:
    ServiceProvider() = default;
    virtual ~ServiceProvider() = default;

    template <typename T, std::enable_if_t<std::is_base_of_v<IService, T>, int> = 0>
    void registerService(std::unique_ptr<T> service, bool overwriteFlag = false)
    {

        registerService(std::type_index(typeid(T)), std::move(service), overwriteFlag);
    }

    /**
     *
     */
    void registerService(std::type_index type, std::unique_ptr<IService> service, bool overwriteFlag = false)
    {
        if (!service)
        {
            std::cerr << "Attempted to register a null service." << std::endl;
            return;
        }
        if (services.find(type) != services.end())
        {
            std::cerr << "Service of type " << type.name() << " is already registered." << std::endl;
            return;
        }
        services[type] = std::move(service);
    }

    /**
     *
     * Retrieves a service of the specified type.
     *
     *
     * Notice that Do Not Call registerService<IService>(...), it's abstract class, Not A Fully Functional Service..
     * @return A pointer to the service if found, otherwise nullptr.
     */
    template <typename T, std::enable_if_t<std::is_base_of_v<IService, T>, int> = 0>
    T *getService()
    {
        auto it = services.find(std::type_index(typeid(T)));
        if (it != services.end())
        {
            return static_cast<T *>(it->second.get());
        }
        return nullptr;
    }

private:
    friend class ServiceInitializer;
    std::unordered_map<std::type_index, std::unique_ptr<IService>> services;
};