#pragma once
#include <string>
#include <unordered_map>
#include <yoga/Yoga.h>

#include <memory>
#include <variant>
#include <vector>

// forward declaration
class SceneGraph;

// Dom Element class - this is the base class for all elements in the DOM tree
class Element
{
public:
    // monostate := null | undefined
    using AttrValue = std::variant<std::monostate, bool, int, float, std::string>;

private:
    YGNodeRef ygNode;

    SceneGraph *sceneGraph = nullptr;

    // Scene Manager will manage and identify elements by their unique ID, which is a string in this case
    uint64_t uid;
    bool dirtyFlag;

    // basic attributes on JS Side (id, text, src, etc.)
    std::string id;
    std::string text;
    std::string src;
    bool visible;

    // other attributes
    std::unordered_map<std::string, Element::AttrValue> attributes;

    // DOM Elem children and parent
    std::vector<Element *> children;
    Element *parent = nullptr;

public:
    Element(uint64_t uid);
    virtual ~Element();

    uint64_t getUid()
    {
        return this->uid;
    }

    void setId(const std::string &id)
    {
        this->id = id;
    }

    const std::string &getId() const
    {
        return id;
    }

    void setText(const std::string &text)
    {
        this->text = text;
    }

    void setVisible(bool visible)
    {
        this->visible = visible;
    }

    const std::string &getText() const
    {
        return text;
    }

    bool getVisible() const
    {
        return visible;
    }

    void setSrc(const std::string &src)
    {
        this->src = src;
    }

    const std::string &getSrc() const
    {
        return src;
    }

    void setParent(Element *parent)
    {
        this->parent = parent;
    }
    Element *getParent()
    {
        return parent;
    }

    void setSceneGraph(SceneGraph *graph)
    {
        this->sceneGraph = graph;
    }

    SceneGraph *getSceneGraph()
    {
        return sceneGraph;
    }

    void setDirtyFlag(bool flag)
    {
        this->dirtyFlag = flag;
    }

    bool getDirtyFlag()
    {
        return dirtyFlag;
    }

    virtual void SetProperty(const std::string &key, const std::string &value);
    // virtual void ApplyAttributes(const std::string &key, const std::string &value);
    virtual void ApplyAttributes(const std::string &key, AttrValue value);
    void appendChild(Element *child);

    void updateSceneRecursively(SceneGraph *sceneGraph);

    // attach renderer or etc component to element,
    void onAttachElement();
};

// struct BoxElement // this is <div>
// {
//     float x0, y0, x1, y1;
// };
// struct TextElement // this is <text>, <p>, etc.
// {
// };

// struct ImageElement // this is <img>
// {
// };