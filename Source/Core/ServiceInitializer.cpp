#include "ServiceInitializer.h"
#include "ServiceProvider.h"
#include "ServiceRegistry.h"

bool ServiceInitializer::createAllServices(ServiceRegistry &registry, ServiceProvider &provider)
{

    for (auto &serviceInfo : registry.GetFactories(ServicePhase::System))
    {
        auto service = serviceInfo.factory();
        provider.registerService(serviceInfo.type, std::move(service));
    }

    for (auto &serviceInfo : registry.GetFactories(ServicePhase::Logic))
    {
        auto service = serviceInfo.factory();
        provider.registerService(serviceInfo.type, std::move(service));
    }

    for (auto &serviceInfo : registry.GetFactories(ServicePhase::Render))
    {
        auto service = serviceInfo.factory();
        provider.registerService(serviceInfo.type, std::move(service));
    }

    return true;
}

bool ServiceInitializer::initializeAllServices(ServiceRegistry &registry, ServiceProvider &provider)
{

    for (auto &[type, service] : provider.services)
    {
        service->initialize(&provider);
    }

    return true;
}
