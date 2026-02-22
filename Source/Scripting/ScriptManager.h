#pragma once
#include <string>
#include <memory>
#include "Core/ComponentRegistry.h"
#include "Core/IFileLoader.h"
#include "Core/UiEngine.h"

// Forward declaration
class SceneManager;
class SceneGraph;
class ScriptManager;

struct ScriptExecutionContext
{
    SceneGraph *defaultSceneGraph;                  // this is main Scene,
    std::vector<SceneGraph *> auxiliarySceneGraphs; // auxilary Scenes
};

class ScriptManager : public IComponent
{

public:
    ScriptManager();
    ~ScriptManager();

    void ReloadScripts();
    void Update();
    void Init();
    void Execute(const std::string &code);
    void ExecuteFile(const std::string &path);
    void ExecuteModule(const std::string &modulePath);

    // RAII Style ScirptExecutionContext Guard
    struct ContextGuard
    {
        ScriptManager *sm;
        ContextGuard(ScriptManager *sm, ScriptExecutionContext *context) : sm(sm)
        {
            // constructor: push new context
            sm->pushContext(context);
        }
        ~ContextGuard()
        {
            // destructor: pop context
            sm->popContext();
        }
    };

    // Context Management
    ScriptExecutionContext *createOrGetExecutionContext(const std::string &modulePath);
    void pushContext(ScriptExecutionContext *context);
    void popContext();
    ScriptExecutionContext *getActiveContext();

    // 엔진이 초기화될 때 호출 (여기서 다른 컴포넌트를 찾거나 초기 설정을 합니다)
    virtual void OnInit(UiEngine *engine) override;
    // 매 프레임 호출
    virtual void OnUpdate() override;

    IFIleLoader *GetFileLoader() { return m_fileLoader; }
    SceneManager *getSceneManager() { return m_sceneManager; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
    std::vector<ScriptExecutionContext *> ScriptExecutionContextStack;

    IFIleLoader *m_fileLoader = nullptr;
    SceneManager *m_sceneManager = nullptr;
    UiEngine *m_engine = nullptr;
};
