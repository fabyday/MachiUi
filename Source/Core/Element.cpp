#include "Element.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <sstream>

static std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });
    return value;
}

static float ParseStyleFloat(const std::string &value)
{
    if (value.empty())
    {
        return 0.0f;
    }

    return std::stof(value);
}

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

static void ApplyOptionalDimensionStyle(YGNodeRef node, const std::string &value,
                                        void (*pointFunc)(YGNodeRef, float),
                                        void (*percentFunc)(YGNodeRef, float))
{
    if (value.empty() || value == "auto")
    {
        return;
    }

    if (value.back() == '%')
    {
        float percentValue = std::stof(value.substr(0, value.size() - 1));
        percentFunc(node, percentValue);
    }
    else
    {
        pointFunc(node, ParseStyleFloat(value));
    }
}

static void ApplyEdgeStyle(YGNodeRef node, const std::string &value, YGEdge edge,
                           void (*pointFunc)(YGNodeRef, YGEdge, float),
                           void (*percentFunc)(YGNodeRef, YGEdge, float))
{
    if (value.empty())
    {
        return;
    }

    if (value.back() == '%')
    {
        float percentValue = std::stof(value.substr(0, value.size() - 1));
        percentFunc(node, edge, percentValue);
    }
    else
    {
        pointFunc(node, edge, ParseStyleFloat(value));
    }
}

static YGFlexDirection ParseFlexDirection(const std::string &value)
{
    const std::string normalized = ToLower(value);
    if (normalized == "row")
    {
        return YGFlexDirectionRow;
    }
    if (normalized == "row-reverse")
    {
        return YGFlexDirectionRowReverse;
    }
    if (normalized == "column-reverse")
    {
        return YGFlexDirectionColumnReverse;
    }
    return YGFlexDirectionColumn;
}

static YGJustify ParseJustify(const std::string &value)
{
    const std::string normalized = ToLower(value);
    if (normalized == "center")
    {
        return YGJustifyCenter;
    }
    if (normalized == "flex-end")
    {
        return YGJustifyFlexEnd;
    }
    if (normalized == "space-between")
    {
        return YGJustifySpaceBetween;
    }
    if (normalized == "space-around")
    {
        return YGJustifySpaceAround;
    }
    if (normalized == "space-evenly")
    {
        return YGJustifySpaceEvenly;
    }
    return YGJustifyFlexStart;
}

static YGAlign ParseAlign(const std::string &value)
{
    const std::string normalized = ToLower(value);
    if (normalized == "auto")
    {
        return YGAlignAuto;
    }
    if (normalized == "center")
    {
        return YGAlignCenter;
    }
    if (normalized == "flex-end")
    {
        return YGAlignFlexEnd;
    }
    if (normalized == "stretch")
    {
        return YGAlignStretch;
    }
    if (normalized == "baseline")
    {
        return YGAlignBaseline;
    }
    if (normalized == "space-between")
    {
        return YGAlignSpaceBetween;
    }
    if (normalized == "space-around")
    {
        return YGAlignSpaceAround;
    }
    if (normalized == "space-evenly")
    {
        return YGAlignSpaceEvenly;
    }
    return YGAlignFlexStart;
}

static YGWrap ParseWrap(const std::string &value)
{
    const std::string normalized = ToLower(value);
    if (normalized == "wrap")
    {
        return YGWrapWrap;
    }
    if (normalized == "wrap-reverse")
    {
        return YGWrapWrapReverse;
    }
    return YGWrapNoWrap;
}

static YGDisplay ParseDisplay(const std::string &value)
{
    const std::string normalized = ToLower(value);
    if (normalized == "none")
    {
        return YGDisplayNone;
    }
    return YGDisplayFlex;
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
    {"minWidth", [](YGNodeRef n, const std::string &v)
     {
         ApplyOptionalDimensionStyle(n, v, YGNodeStyleSetMinWidth, YGNodeStyleSetMinWidthPercent);
     }},
    {"minHeight", [](YGNodeRef n, const std::string &v)
     {
         ApplyOptionalDimensionStyle(n, v, YGNodeStyleSetMinHeight, YGNodeStyleSetMinHeightPercent);
     }},
    {"maxWidth", [](YGNodeRef n, const std::string &v)
     {
         ApplyOptionalDimensionStyle(n, v, YGNodeStyleSetMaxWidth, YGNodeStyleSetMaxWidthPercent);
     }},
    {"maxHeight", [](YGNodeRef n, const std::string &v)
     {
         ApplyOptionalDimensionStyle(n, v, YGNodeStyleSetMaxHeight, YGNodeStyleSetMaxHeightPercent);
     }},
    {"flexBasis", [](YGNodeRef n, const std::string &v)
     {
         ApplyDimensionStyle(n, v, YGNodeStyleSetFlexBasis, YGNodeStyleSetFlexBasisPercent, YGNodeStyleSetFlexBasisAuto);
     }},
    {"flex", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetFlex(n, ParseStyleFloat(v));
     }},
    {"flexGrow", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetFlexGrow(n, ParseStyleFloat(v));
     }},
    {"flexShrink", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetFlexShrink(n, ParseStyleFloat(v));
     }},
    {"flexDirection", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetFlexDirection(n, ParseFlexDirection(v));
     }},
    {"flexWrap", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetFlexWrap(n, ParseWrap(v));
     }},
    {"justifyContent", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetJustifyContent(n, ParseJustify(v));
     }},
    {"alignItems", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetAlignItems(n, ParseAlign(v));
     }},
    {"alignSelf", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetAlignSelf(n, ParseAlign(v));
     }},
    {"alignContent", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetAlignContent(n, ParseAlign(v));
     }},
    {"gap", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetGap(n, YGGutterAll, ParseStyleFloat(v));
     }},
    {"rowGap", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetGap(n, YGGutterRow, ParseStyleFloat(v));
     }},
    {"columnGap", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetGap(n, YGGutterColumn, ParseStyleFloat(v));
     }},
    {"padding", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeAll, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
     }},
    {"paddingHorizontal", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeHorizontal, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
     }},
    {"paddingVertical", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeVertical, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
     }},
    {"paddingLeft", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeLeft, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
     }},
    {"paddingTop", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeTop, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
     }},
    {"paddingRight", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeRight, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
     }},
    {"paddingBottom", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeBottom, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
     }},
    {"margin", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeAll, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent);
     }},
    {"marginHorizontal", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeHorizontal, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent);
     }},
    {"marginVertical", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeVertical, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent);
     }},
    {"marginLeft", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeLeft, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent);
     }},
    {"marginTop", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeTop, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent);
     }},
    {"marginRight", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeRight, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent);
     }},
    {"marginBottom", [](YGNodeRef n, const std::string &v)
     {
         ApplyEdgeStyle(n, v, YGEdgeBottom, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent);
     }},
    {"aspectRatio", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetAspectRatio(n, ParseStyleFloat(v));
     }},
    {"display", [](YGNodeRef n, const std::string &v)
     {
         YGNodeStyleSetDisplay(n, ParseDisplay(v));
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

Element::Element(uint64_t uid) : uid(uid), dirtyFlag(false), visible(true)
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
        if ((key == "fontSize" || key == "fontFamily" || key == "lineHeight") && YGNodeHasMeasureFunc(ygNode))
        {
            YGNodeMarkDirty(ygNode);
        }
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
