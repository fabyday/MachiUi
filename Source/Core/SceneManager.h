#pragma once
#include <unordered_map>
#include <memory>
#include "Element.h"
#include "SceneGraph.h"
#include "IComponent.h"
#include "ElementFactory.h"

class SceneManager : public IComponent
{

    ElementFactory *elementFactory = nullptr;

    // Zero is reserved for null pointer, so we start from 1
    uint64_t nextElementId = 1;

    /// object pool for all elements, key is pointer value of element, value is unique_ptr of element
    std::unordered_map<uint64_t, std::unique_ptr<Element>> objectPool;
    std::unordered_map<uint64_t, std::unique_ptr<SceneGraph>> sceneGraphMap;

protected:
    uint64_t generateElementId();

public:
    SceneManager();
    ~SceneManager();

    // IComponent interface implementation
    virtual void OnInit(UiEngine *engine) override;
    virtual void OnUpdate() override;

    // Scene Graph management(create, destroy, get)
    uint64_t createSceneGraph(const std::string &sceneName);
    void destroySceneGraph(const uint64_t Id);
    SceneGraph *getSceneGraph(const uint64_t Id);

    // CreateRoot
    bool createRoot(uint64_t SceneGraphId);

    // Element management(create, destroy, get)
    Element *createElement(const std::string &type);
    void destroyElement(const uint64_t Id);
    void destroyAllChildren(const uint64_t Id);
    Element *getElement(const uint64_t Id);

    // check if
    bool isMounted(uint64_t elementId);
};
