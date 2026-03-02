#include "SceneManager.h"
#include "ComponentRegistry.h"
#include "UiEngine.h"
#include "SceneGraph.h"
#include "LogManager.h"
#include "ILogger.h"

SceneManager::SceneManager() : nextElementId(1)
{
}
SceneManager::~SceneManager()
{
}

void SceneManager::OnInit(UiEngine *engine)
{
    this->elementFactory = engine->GetComponent<ElementFactory>();
    this->logManager = engine->GetComponent<LogManager>();
}

void SceneManager::OnUpdate()
{
}

uint64_t SceneManager::generateElementId()
{

    return nextElementId++;
}

uint64_t SceneManager::createSceneGraph(const std::string &sceneName)
{
    this->logManager->getLogger()->LogDebug("{}", "create SceneGraph");
    const uint64_t sceneGraphId = this->generateElementId();

    sceneGraphMap[sceneGraphId] = std::make_unique<SceneGraph>(sceneGraphId, sceneName, this);

    return sceneGraphId;
}

void SceneManager::destroySceneGraph(const uint64_t Id)
{
    this->logManager->getLogger()->LogDebug("{}", "destroy SceneGraph");
    auto it = sceneGraphMap.find(Id);
    if (it != sceneGraphMap.end())
    {
        sceneGraphMap.erase(it);
    }
}

SceneGraph *SceneManager::getSceneGraph(const uint64_t Id)
{
    auto it = sceneGraphMap.find(Id);
    if (it != sceneGraphMap.end())
    {
        return it->second.get();
    }
    return nullptr; // 씬 그래프가 존재하지 않을 때 nullptr 반환
}

bool SceneManager::createRoot(uint64_t SceneGraphId)
{
    SceneGraph *graph = this->getSceneGraph(SceneGraphId);
    if (!graph)
    {
        this->logManager->getLogger()->LogError("{} {}", SceneGraphId, "create SceneGraph");
        return false;
    }

    Element *rootElement = this->createElement("Root");
    if (!rootElement)
    {
        this->logManager->getLogger()->LogError("{} : {}", "Failed to create root element for SceneGraph ID", SceneGraphId);
        return false;
    }
    graph->setRoot(rootElement);
    return true;
}

Element *SceneManager::createElement(const std::string &type)
{
    const uint64_t elementId = this->generateElementId();
    auto element = this->elementFactory->create(type, elementId);

    if (element == nullptr)
    {
        this->logManager->getLogger()->LogError("{} : {}", "Failed to create element of type", type);
        return nullptr;
    }

    Element *rawElementPtr = element.get();
    objectPool[elementId] = std::move(element);

    return rawElementPtr;
}

void SceneManager::destroyElement(const uint64_t Id)
{
    auto it = objectPool.find(Id);
    if (it != objectPool.end())
    {
        objectPool.erase(it);
    }
}

Element *SceneManager::getElement(const uint64_t Id)
{
    auto it = objectPool.find(Id);
    if (it != objectPool.end())
    {
        return it->second.get();
    }
    return nullptr; // 요소가 존재하지 않을 때 nullptr 반환
}

void SceneManager::destroyAllChildren(const uint64_t Id)
{
    // TODO: 구현 필요
}

void SceneManager::attachElementToGraph(uint64_t graphId, uint64_t elementId)
{
    SceneGraph *graph = this->getSceneGraph(graphId);
    if (!graph)
    {
    }
}

void SceneManager::detachElementToGraph(uint64_t graphId, uint64_t elementId)
{
}

void SceneManager::removeElementToGraph(uint64_t graphId, uint64_t elementId)
{
}

void SceneManager::updateAttribute(uint64_t ElementId, const std::string &key, const Element::AttrValue &value)
{
    Element *element = this->getElement(ElementId);
    if (element)
    {
        element->ApplyAttributes(key, value);
        element->setDirtyFlag(true);
        this->dirtyElementLists.push_back(ElementId);
    }
}

bool SceneManager::isMounted(uint64_t elementId)
{
    auto elem = this->getElement(elementId);
    return true;
    
}

REGISTER_UI_COMPONENT(SceneManager, ComponentPhase::System);