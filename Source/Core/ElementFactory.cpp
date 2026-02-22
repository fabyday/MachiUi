#include "./ElementFactory.h"
#include "ComponentRegistry.h"
#include <iostream>

ElementFactory::ElementFactory()
{
    std::cout << "ElementFactory created" << std::endl;
}

ElementFactory::~ElementFactory()
{
}

// IComponent interface implementation
void ElementFactory::OnInit(UiEngine *engine)
{
}

void ElementFactory::OnUpdate()
{
}

// ElementFactory methods

void ElementFactory::registerElementType(const std::string &type, ElementCreatorFunc creator)
{
    registry[type] = creator;
}
std::unique_ptr<Element> ElementFactory::create(const std::string &type, uint64_t uid)
{
    auto it = registry.find(type);
    if (it != registry.end())
    {
        return it->second(uid);
    }
    return nullptr; // 등록되지 않은 타입일 경우 nullptr 반환
}

REGISTER_UI_COMPONENT(ElementFactory, ComponentPhase::System);