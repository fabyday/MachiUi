#include "SceneGraph.h"
#include "SceneManager.h"
#include <iostream>

SceneGraph::SceneGraph(uint64_t id, const std::string &name, SceneManager *manager) : uid(id), name(name), manager(manager)
{
    // now root element is created when scene graph is created, it will be created when js createRoot is called, and it will be attached to scene graph later
    //  root = std::make_unique<Element>();
}

SceneGraph::~SceneGraph()
{
    // SceneGraph가 파괴될 때, root Element도 함께 파괴됨
    // root.reset(); // unique_ptr이므로 자동으로 해제됨
}

bool SceneGraph::setRoot(Element *Root)
{
    if (root != nullptr)
    {
        std::cerr << "Root element already exists for SceneGraph ID: " << uid << std::endl;
        return false; // 이미 루트 요소가 존재하는 경우 실패
    }
    this->root = Root;
    root->setParent(nullptr);
    
    return true;
}

