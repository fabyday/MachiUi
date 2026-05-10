#include "Element.h"
#include <algorithm>
#include <functional>
#include <sstream>
// helper function
void ApplyDimensionStyle(YGNodeRef node, const std::string &value,
                         void (*pointFunc)(YGNodeRef, float),
                         void (*percentFunc)(YGNodeRef, float),
                         void (*autoFunc)(YGNodeRef))
{
    if (value.empty())
    {
        return;
    }

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

using AttrFunc = std::function<void(Element *, const Element::AttrValue &)>;

static std::string AttrValueToString(const Element::AttrValue &value)
{
    if (const auto *stringValue = std::get_if<std::string>(&value))
    {
        return *stringValue;
    }
    if (const auto *intValue = std::get_if<int>(&value))
    {
        return std::to_string(*intValue);
    }
    if (const auto *floatValue = std::get_if<float>(&value))
    {
        std::ostringstream stream;
        stream << *floatValue;
        return stream.str();
    }
    if (const auto *boolValue = std::get_if<bool>(&value))
    {
        return *boolValue ? "true" : "false";
    }
    return "";
}

/**
 * 속성 맵: 문자열 키를 멤버 함수에 매핑
 */
static std::unordered_map<std::string, AttrFunc> attrMap = {
    {"text", [](Element *elem, const Element::AttrValue &value)
     {
         const std::string *valueString = std::get_if<std::string>(&value);
         elem->setText(valueString != nullptr ? *valueString : "");
     }

    },
    {"id", [](Element *elem, const Element::AttrValue &value)
     {
         const std::string *valueString = std::get_if<std::string>(&value);

         elem->setId(valueString != nullptr ? *valueString : "");
     }

    },
    {"src", [](Element *elem, const Element::AttrValue &value)
     {
         const std::string *valueString = std::get_if<std::string>(&value);
         elem->setSrc(valueString != nullptr ? *valueString : "");
     }

    },
    {"visible", [](Element *elem, const Element::AttrValue &value)
     {
         if (std::holds_alternative<std::string>(value))
         {
             auto valueString = std::get<std::string>(value);
             elem->setVisible(valueString == "true" || valueString == "1");
         }
         else
         {
             const bool *valueBool = std::get_if<bool>(&value);
             elem->setVisible(valueBool == nullptr ? true : *valueBool);
         }
     }},
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
    if (value.empty())
    {
        return;
    }

    bool isAuto = (value == "auto");
    bool isPercent = (value.back() == '%');
    float numValue = isPercent ? std::stof(value.substr(0, value.size() - 1)) / 100.0f : std::stof(value);
    
    styleMap[key](node, value);
}

// attribute apply helper class
class AttributeApplier
{
public:
    static void ApplyAttribute(Element *element, const std::string &key, const Element::AttrValue &value);
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

void AttributeApplier::ApplyAttribute(Element *element, const std::string &key, const Element::AttrValue &value)
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

const Element::AttrValue *Element::getAttribute(const std::string &key) const
{
    auto it = attributes.find(key);
    if (it == attributes.end())
    {
        return nullptr;
    }

    return &it->second;
}

/**
 *
 */
// void Element::ApplyAttributes(const std::string &key, const std::string &value)
void Element::ApplyAttributes(const std::string &key, Element::AttrValue value)
{
    if (StyleApplier::HasKey(key))
    {
        StyleApplier::ApplyStyle(ygNode, key, AttrValueToString(value));
    }
    else if (AttributeApplier::HasKey(key))
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
    if (child == nullptr)
    {
        return;
    }

    if (child->parent != nullptr)
    {
        child->parent->removeChild(child);
    }

    children.push_back(child);
    child->setParent(this);
    child->updateSceneRecursively(sceneGraph);

    YGNodeInsertChild(this->ygNode, child->ygNode, YGNodeGetChildCount(this->ygNode));
}

void Element::insertChildBefore(Element *child, Element *beforeChild)
{
    if (child == nullptr)
    {
        return;
    }

    if (beforeChild == nullptr)
    {
        appendChild(child);
        return;
    }

    if (child->parent != nullptr)
    {
        child->parent->removeChild(child);
    }

    auto beforeIt = std::find(children.begin(), children.end(), beforeChild);
    if (beforeIt == children.end())
    {
        appendChild(child);
        return;
    }

    const auto index = static_cast<uint32_t>(std::distance(children.begin(), beforeIt));
    children.insert(children.begin() + index, child);
    child->setParent(this);
    child->updateSceneRecursively(sceneGraph);
    YGNodeInsertChild(this->ygNode, child->ygNode, index);
}

void Element::removeChild(Element *child)
{
    if (child == nullptr)
    {
        return;
    }

    auto it = std::find(children.begin(), children.end(), child);
    if (it == children.end())
    {
        return;
    }

    YGNodeRemoveChild(this->ygNode, child->ygNode);
    child->setParent(nullptr);
    child->setSceneGraph(nullptr);
    children.erase(it);
}

void Element::removeAllChildren()
{
    for (Element *child : children)
    {
        if (child != nullptr)
        {
            YGNodeRemoveChild(this->ygNode, child->ygNode);
            child->setParent(nullptr);
            child->setSceneGraph(nullptr);
        }
    }
    children.clear();
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

    this->sceneGraph = sceneGraph;
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
