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



