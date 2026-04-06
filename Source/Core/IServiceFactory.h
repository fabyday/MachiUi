#pragma once

// forward declaration
class Engine;
class IService;
class IServiceFactory
{
public:
    virtual ~IServiceFactory() = default;

    virtual IService *createService(Engine *engine) = 0;
};

template <typename ServiceClassName, typename... Args>
IService *serviceFactoryCreateHelper(Engine *engine)
{
    return new ServiceClassName(engine->GetService<Args>()...);
}

#define SERVICE_CONSTRUCTOR(ServiceClassName, ...)

ServiceClassName(__VA_ARGS__) {\
\
}\
class ServiceFactory : public IServiceFactory\
{
public:
    virtual IService *createService(Engine *engine) override { return serviceFactoryCreateHelper<ServiceClassName, __VA_ARGS__>(engine); }
};
static ServiceFactory global_##Service##Factory;
