#include "Element.h"
#include <functional>
// helper function
void ApplyDimensionStyle(YGNodeRef node, const std::string &value,
                         void (*pointFunc)(YGNodeRef, float),
                         void (*percentFunc)(YGNodeRef, float),
                         void (*autoFunc)(YGNodeRef))
{
    if (value == "auto")
    {
        autoFunc(node);
    }
    else if (value.back() == '%')
    {
        float percentValue = std::stof(value.substr(0, value.size() - 1));
        percentFunc(node, percentValue);
    }
    else
    {
        float pointValue = std::stof(value);
        pointFunc(node, pointValue);
    }
}
using StyleFunc = std::function<void(YGNodeRef, const std::string &)>;

std::unordered_map<std::string, StyleFunc> styleMap = {
    {"width", [](YGNodeRef n, const std::string &v)
     {
         ApplyDimensionStyle(n, v, YGNodeStyleSetWidth, YGNodeStyleSetWidthPercent, YGNodeStyleSetWidthAuto);
     }},
    {"height", [](YGNodeRef n, const std::string &v)
     {
         ApplyDimensionStyle(n, v, YGNodeStyleSetHeight, YGNodeStyleSetHeightPercent, YGNodeStyleSetHeightAuto);
     }},
    {"flexBasis", [](YGNodeRef n, const std::string &v)
     {
         ApplyDimensionStyle(n, v, YGNodeStyleSetFlexBasis, YGNodeStyleSetFlexBasisPercent, YGNodeStyleSetFlexBasisAuto);
     }}};

using AttrFunc = std::function<void(Element *, const std::string &)>;

/**
 * 속성 맵: 문자열 키를 멤버 함수에 매핑
 */
static std::unordered_map<std::string, AttrFunc> attrMap = {
    {"text", [](Element *elem, const std::string &value)
     {
         elem->setText(value);
     }

    },
    {"id", [](Element *elem, const std::string &value)
     {
         elem->setId(value);
     }

    },
    {"src", [](Element *elem, const std::string &value)
     {
         elem->setSrc(value);
     }

    },
    {"visible", [](Element *elem, const std::string &value)
     {
         elem->setVisible(value == "true" || value == "1");
     }

    },
};

// style apply helper class
class StyleApplier
{
public:
    static void ApplyStyle(YGNodeRef node, const std::string &key, const std::string &value);
    static bool HasKey(const std::string &key);
    static std::vector<std::string> GetAllStyleKeys();
};

std::vector<std::string> StyleApplier::GetAllStyleKeys()
{
    std::vector<std::string> keys;
    for (const auto &pair : styleMap)
    {
        keys.push_back(pair.first);
    }
    return keys;
}

bool StyleApplier::HasKey(const std::string &key)
{
    return styleMap.find(key) != styleMap.end();
}

void StyleApplier::ApplyStyle(YGNodeRef node, const std::string &key, const std::string &value)
{
    bool isAuto = (value == "auto");
    bool isPercent = (value.back() == '%');
    float numValue = isPercent ? std::stof(value.substr(0, value.size() - 1)) / 100.0f : std::stof(value);

    styleMap[key](node, value);
}

// attribute apply helper class
class AttributeApplier
{
public:
    static void ApplyAttribute(Element *element, const std::string &key, const std::string &value);
    static bool HasKey(const std::string &key);
    static std::vector<std::string> GetAllStyleKeys();
};

std::vector<std::string> AttributeApplier::GetAllStyleKeys()
{
    std::vector<std::string> keys;
    for (const auto &pair : attrMap)
    {
        keys.push_back(pair.first);
    }
    return keys;
}

bool AttributeApplier::HasKey(const std::string &key)
{
    return attrMap.find(key) != attrMap.end();
}

void AttributeApplier::ApplyAttribute(Element *element, const std::string &key, const std::string &value)
{
    attrMap[key](element, value);
}

Element::Element(uint64_t uid) : uid(uid)
{
    ygNode = YGNodeNew();
    YGNodeSetContext(ygNode, this);
}

Element::~Element()
{
    if (this->ygNode)
    {
        YGNodeFree(this->ygNode);
    }
}

void Element::SetProperty(const std::string &key, const std::string &value)
{
    if (StyleApplier::HasKey(key))
    {
        StyleApplier::ApplyStyle(ygNode, key, value);
    }
    else
    {
        this->ApplyAttributes(key, value);
    }
}

void Element::ApplyAttributes(const std::string &key, const std::string &value)
{
    if (AttributeApplier::HasKey(key))
    {
        AttributeApplier::ApplyAttribute(this, key, value);
    }
    else
    {
        attributes[key] = value;
    }
}

// append Dom Element child
void Element::appendChild(Element *child)
{
    // ownership transfer to children vector
    children.push_back(child);
    Element *childPtr = children.back();

    YGNodeInsertChild(this->ygNode, childPtr->ygNode, YGNodeGetChildCount(this->ygNode));
}

/**
 * SceneGraph must not be nullptr when calling this function,
 * it should be called after element is attached to scene graph
 * Non-nullable is garanteed by SceneManager.
 */
void Element::updateSceneRecursively(SceneGraph *sceneGraph)
{

    // If the element is already attached to the same scene graph,
    // no need to update again
    if (this->sceneGraph == sceneGraph)
    {
        return;
    }

    // 자식 요소들도 재귀적으로 업데이트
    for (Element *child : children)
    {
        child->updateSceneRecursively(sceneGraph);
    }
}

void Element::onAttachElement()
{
    this->setParent(parent);
    this->setSceneGraph(sceneGraph);

    // // propagate scene graph and parent info to children recursively
    // for (Element *grandChild : this->children)
    // {
    //     grandChild->onAttachElement(this, this->sceneGraph);
    // }
}