#include "./ElementFactory.h"
#include "ComponentRegistry.h"
#include "UiEngine.h"
#include "LogManager.h"
// default Elements
#include "../Elements/Div.h"
#include "../Elements/Button.h"
#include "../Elements/Img.h"
#include "../Elements/Text.h"
#include "../Elements/Span.h"

inline static void initializeBasicElements(ElementFactory *elementFactory);

ElementFactory::ElementFactory()
{
}

ElementFactory::~ElementFactory()
{
}

// IComponent interface implementation
void ElementFactory::onInit(UiEngine *engine)
{
    logManager = engine->GetService<LogManager>();
    logger = logManager->getLogger();

    MACHI_LOG_INFO(logger, "{}", "Initialize ElementFactory")

    initializeBasicElements(this);
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

// Init Basic Elements
static void initializeBasicElements(ElementFactory *elementFactory)
{
    elementFactory->registerElementType("div", [](uint64_t uid)
                                        { return std::make_unique<DivElement>(uid); });
    elementFactory->registerElementType("img", [](uint64_t uid)
                                        { return std::make_unique<ImgElement>(uid); });
    elementFactory->registerElementType("text", [](uint64_t uid)
                                        { return std::make_unique<TextElement>(uid); });
    elementFactory->registerElementType("button", [](uint64_t uid)
                                        { return std::make_unique<ButtonElement>(uid); });
    elementFactory->registerElementType("span", [](uint64_t uid)
                                        { return std::make_unique<SpanElement>(uid); });
};

REGISTER_UI_COMPONENT(ElementFactory, ServicePhase::System);