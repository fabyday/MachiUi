#pragma once
#include <unordered_map>
#include <memory>
#include "Element.h"
#include "SceneGraph.h"
#include "IService.h"
#include "ElementFactory.h"
#include "IServiceFactory.h"
class LogManager;

class ServiceProvider;

class SceneManager : public IService
{

    // manager dependency
    LogManager *logManager = nullptr;

    // helper
    ElementFactory *elementFactory = nullptr;

    // Zero is reserved for null pointer, so we start from 1
    uint64_t nextElementId = 1;

    std::vector<uint64_t> dirtyElementLists;

    /// object pool for all elements, key is pointer value of element, value is unique_ptr of element
    std::unordered_map<uint64_t, std::unique_ptr<Element>> objectPool;
    std::unordered_map<uint64_t, std::unique_ptr<SceneGraph>> sceneGraphMap;

protected:
    uint64_t generateElementId();

public:
    SceneManager();
    ~SceneManager();

    // IComponent interface implementation
    virtual void onInit(ServiceProvider *provider) override;

    // Scene Graph management(create, destroy, get)
    uint64_t createSceneGraph(const std::string &sceneName);
    void destroySceneGraph(const uint64_t Id);
    SceneGraph *getSceneGraph(const uint64_t Id);

    // CreateRoot
    bool createRoot(uint64_t SceneGraphId);

    // Element management(create, destroy, get)
    Element *createElement(const std::string &type);
    void destroyElement(const uint64_t Id);
    void AppendElement(const uint64_t parentId, const uint64_t childId);
    void destroyAllChildren(const uint64_t Id);
    Element *getElement(const uint64_t Id);

    // Element and Graph manipulation
    void attachElementToGraph(uint64_t graphId, uint64_t elementId);
    void detachElementToGraph(uint64_t graphId, uint64_t elementId);
    void removeElementToGraph(uint64_t graphId, uint64_t elementId);

    void updateAttribute(uint64_t ElementId, const std::string &key, const Element::AttrValue &value);

    // check if
    bool isMounted(uint64_t elementId);
};
