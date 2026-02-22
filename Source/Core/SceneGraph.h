#pragma once
#include "Element.h"
#include <memory>

// forward declaration
class SceneManager;

class SceneGraph
{
public:
    enum class SceneMode
    {
        Attached, // attahced to Main window,
        Detached, // separate main window
    };

private:
    uint64_t uid;
    std::string name;
    bool dirty = true;
    Element *root = nullptr;

    SceneMode mode = SceneMode::Attached;

    // SceneManager injected when SceneGraph is created,
    // it can be used to access SceneManager's functionalities if needed,
    // but it should be used carefully to avoid tight coupling
    SceneManager *manager = nullptr;

public:
    SceneGraph(uint64_t uid, const std::string &name, SceneManager *manager);
    ~SceneGraph();

    uint64_t getUid() const { return uid; }
    const std::string &getName() const { return name; }
    Element *getRoot() const { return root; };
    bool setRoot(Element *Root);

private:
};