#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include "Element.h"
#include "IService.h"

class LogManager;
class ILogger;

class ServiceProvider;

using ElementCreatorFunc = std::function<std::unique_ptr<Element>(uint64_t uid)>;

// @brief ElementFactory is responsible for creating Element instances
// based on type strings. It maintains a registry of type strings to creator functions,
// allowing for flexible and extensible element creation.
class ElementFactory : public IService
{

private:
    std::unordered_map<std::string, ElementCreatorFunc> registry;
    LogManager *logManager = nullptr;
    ILogger *logger = nullptr;

public:
    ElementFactory();
    ~ElementFactory();

    // IComponent interface implementation
    virtual void onInit(ServiceProvider *provider) override;

    void registerElementType(const std::string &type, ElementCreatorFunc creator);
    std::unique_ptr<Element> create(const std::string &type, uint64_t uid);
};
