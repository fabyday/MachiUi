#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include "Element.h"
#include "IComponent.h"
using ElementCreatorFunc = std::function<std::unique_ptr<Element>(uint64_t uid)>;

// @brief ElementFactory is responsible for creating Element instances
// based on type strings. It maintains a registry of type strings to creator functions,
// allowing for flexible and extensible element creation.
class ElementFactory : public IComponent
{

private:
    std::unordered_map<std::string, ElementCreatorFunc> registry;

public:
    ElementFactory();
    ~ElementFactory();

    // IComponent interface implementation
    virtual void OnInit(UiEngine *engine) override;
    virtual void OnUpdate() override;

    void registerElementType(const std::string &type, ElementCreatorFunc creator);
    std::unique_ptr<Element> create(const std::string &type, uint64_t uid);
};
