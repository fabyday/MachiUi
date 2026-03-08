#include "ScriptManager.h"
#include "../Core/SceneManager.h"
#include "../Core/LogManager.h"
struct JSModuleDef;
class JSRuntime;
class JSContext;

// foward declaration
struct
    ScriptManager::Impl
{
    JSRuntime *runtime;
    JSContext *context;
    Impl(ScriptManager *manager);
    ~Impl();
    void run(const std::string &code, const std::string &name, bool isModule);
    void handleException();
};

ScriptManager::ScriptManager() : m_pImpl(std::make_unique<Impl>(this))
{
}
ScriptManager::~ScriptManager() = default;

void ScriptManager::Init()
{
}

void ScriptManager::ReloadScripts()
{
}
void ScriptManager::Update()
{
}

void ScriptManager::onInit(UiEngine *engine)
{
    this->m_engine = engine;

    this->m_fileLoader = engine->GetService<IFIleLoader>();

    if (!this->m_fileLoader)
    {
        std::cerr << "[ScriptManager] Critical: IFileLoader not found!" << std::endl;
    }

    this->m_sceneManager = engine->GetService<SceneManager>();
    this->logManager = engine->GetService<LogManager>();
}

// void ScriptManager::OnUpdate()
// {
//     this->ProcessPendingTasks(16);
//     this->ExecuteFrameUpdate(0.1);
// }

void ScriptManager::ProcessPendingTasks(double delta)
{
}
void ScriptManager::ExecuteFrameUpdate(float dt)
{
}

void ScriptManager::Execute(const std::string &code)
{
    m_pImpl->run(code, "<input>", false);
}

void ScriptManager::ExecuteFile(const std::string &path)
{
    if (!m_fileLoader)
        return;

    auto code = m_fileLoader->readFile(path);
    if (code)
    {
        // 파일 내용을 일반 전역 실행
        m_pImpl->run(*code, path, false);
    }
}

void ScriptManager::ExecuteModule(const std::string &modulePath)
{
    if (auto code = m_fileLoader->readFile(modulePath))
    {
        uint64_t sceneId = m_sceneManager->createSceneGraph(modulePath);
        SceneGraph *defaultSceneGraph = m_sceneManager->getSceneGraph(sceneId);
        ScriptExecutionContext *context = new ScriptExecutionContext{defaultSceneGraph, {}};
        ContextGuard guard(this, context);
        m_pImpl->run(*code, modulePath, true);
    }
}

void ScriptManager::pushContext(ScriptExecutionContext *context)
{
    ScriptExecutionContextStack.push_back(context);
}

void ScriptManager::popContext()
{
    if (!ScriptExecutionContextStack.empty())
    {
        ScriptExecutionContextStack.pop_back();
    }
}

ScriptExecutionContext *ScriptManager::getActiveContext()
{
    return ScriptExecutionContextStack.back();
}

ScriptExecutionContext *ScriptManager::createOrGetExecutionContext(const std::string &modulePath)
{
    // TODO
    return nullptr;
}

REGISTER_UI_COMPONENT(ScriptManager, ServicePhase::Logic);
