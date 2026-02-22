#include "SceneManager.h"
#include "ComponentRegistry.h"
#include "UiEngine.h"
#include "SceneGraph.h"

SceneManager::SceneManager() : nextElementId(1)
{
}
SceneManager::~SceneManager()
{
}

void SceneManager::OnInit(UiEngine *engine)
{
    this->elementFactory = engine->GetComponent<ElementFactory>();
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

    const uint64_t sceneGraphId = this->generateElementId();

    sceneGraphMap[sceneGraphId] = std::make_unique<SceneGraph>(sceneGraphId, sceneName, this);

    return sceneGraphId;
}

void SceneManager::destroySceneGraph(const uint64_t Id)
{
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
        std::cerr << "SceneGraph with ID " << SceneGraphId << " not found." << std::endl;
        return false;
    }

    Element *rootElement = this->createElement("Root");
    if (!rootElement)
    {
        std::cerr << "Failed to create root element for SceneGraph ID: " << SceneGraphId << std::endl;
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
        std::cerr << "Failed to create element of type: " << type << std::endl;
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
    //TODO: 구현 필요
}


REGISTER_UI_COMPONENT(SceneManager, ComponentPhase::System);