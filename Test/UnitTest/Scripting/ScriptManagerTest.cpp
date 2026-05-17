#include <gtest/gtest.h>
#include <Scripting/ScriptManager.h>
#include <Core/ActionRegistry.h>
#include <Core/DefaultFileLoader.h>
#include <Core/ElementFactory.h>
#include <Core/EventBus.h>
#include <Core/InputManager.h>
#include <Core/LogManager.h>
#include <Core/NativeViewRegistry.h>
#include <Core/SceneGraph.h>
#include <Core/SceneManager.h>
#include <Core/ServiceProvider.h>
#include <yoga/Yoga.h>

#include <sstream>
#include <string>

TEST(ScriptManagerTest, ContextStackPushPop) {
    // ScriptManager TEST

    // init ScriptManager, deps inject from mock 
    ScriptManager sm ; 
    ScriptExecutionContext ctx;


    sm.pushContext(&ctx);
    // do something 
    sm.getActiveContext();
    GTEST_ASSERT_EQ(1, 1);
    sm.popContext();

    
}

TEST(ScriptManagerTest, NetworkCapabilityDefaultsToDisabled)
{
    ScriptManager sm;

    EXPECT_FALSE(sm.isNetworkEnabled());

    sm.setNetworkEnabled(true);
    EXPECT_TRUE(sm.isNetworkEnabled());

    sm.setNetworkEnabled(false);
    EXPECT_FALSE(sm.isNetworkEnabled());
}

TEST(ActionRegistryTest, InvokesRegisteredAction)
{
    ActionRegistry registry;
    registry.registerAction("panel.create", [](const ActionRequest &request) {
        EXPECT_EQ(request.name, "panel.create");
        EXPECT_EQ(request.payloadJson, "{\"type\":\"inspector\"}");
        return ActionResult::Success("{\"created\":true}");
    });

    ActionResult result = registry.invoke("panel.create", "{\"type\":\"inspector\"}");

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.payloadJson, "{\"created\":true}");
}

TEST(ActionRegistryTest, MissingActionReturnsFailure)
{
    ActionRegistry registry;

    ActionResult result = registry.invoke("missing.action", "null");

    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("missing.action"), std::string::npos);
}

class RecordingNativeViewAdapter : public INativeViewAdapter
{
public:
    int mountCount = 0;
    int updateCount = 0;
    int unmountCount = 0;
    int inputCount = 0;
    NativeViewSlot lastSlot;
    NativeViewInputEvent lastInput;

    void mount(const NativeViewSlot &slot) override
    {
        ++mountCount;
        lastSlot = slot;
    }

    void update(const NativeViewSlot &slot) override
    {
        ++updateCount;
        lastSlot = slot;
    }

    void unmount() override
    {
        ++unmountCount;
    }

    bool dispatchInput(const NativeViewInputEvent &event) override
    {
        ++inputCount;
        lastInput = event;
        return true;
    }
};

TEST(NativeViewRegistryTest, SyncsSlotAndDispatchesInputToAdapter)
{
    NativeViewRegistry registry;
    RecordingNativeViewAdapter *adapter = nullptr;

    registry.registerFactory("game.viewport", [&](const NativeViewSlot &) {
        auto instance = std::make_unique<RecordingNativeViewAdapter>();
        adapter = instance.get();
        return instance;
    });

    NativeViewSlot slot;
    slot.elementId = 42;
    slot.nativeType = "game.viewport";
    slot.viewId = "main";
    slot.rect = Rect{10.0f, 20.0f, 300.0f, 200.0f};
    slot.visible = true;

    registry.beginSync();
    registry.syncSlot(slot);
    registry.endSync();

    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->mountCount, 1);
    EXPECT_EQ(adapter->updateCount, 1);
    EXPECT_EQ(adapter->lastSlot.viewId, "main");

    MouseEvent mouseDown;
    mouseDown.type = MouseEvent::Type::Down;
    mouseDown.x = 30.0f;
    mouseDown.y = 50.0f;
    mouseDown.button = MouseButton::Left;
    mouseDown.isPressed = true;

    EXPECT_TRUE(registry.dispatchMouseEvent(mouseDown));
    EXPECT_EQ(adapter->inputCount, 1);
    EXPECT_EQ(adapter->lastInput.type, NativeViewInputEvent::Type::PointerDown);
    EXPECT_FLOAT_EQ(adapter->lastInput.localX, 20.0f);
    EXPECT_FLOAT_EQ(adapter->lastInput.localY, 30.0f);
}

TEST(MouseDispatchTest, ClickTargetsTopmostRenderedElementAndBubbles)
{
    ServiceProvider provider;
    provider.registerService<LogManager>(std::make_unique<LogManager>());
    provider.registerService<EventBus>(std::make_unique<EventBus>());
    provider.registerService<InputManager>(std::make_unique<InputManager>());
    provider.registerService<ElementFactory>(std::make_unique<ElementFactory>());
    provider.registerService<SceneManager>(std::make_unique<SceneManager>());
    provider.registerService<ActionRegistry>(std::make_unique<ActionRegistry>());
    provider.registerService<NativeViewRegistry>(std::make_unique<NativeViewRegistry>());
    provider.registerService(std::type_index(typeid(IFIleLoader)), std::make_unique<DefaultFileLoader>());
    provider.registerService<ScriptManager>(std::make_unique<ScriptManager>());

    provider.getService<LogManager>()->initialize(&provider);
    provider.getService<EventBus>()->initialize(&provider);
    provider.getService<InputManager>()->initialize(&provider);
    provider.getService<ElementFactory>()->initialize(&provider);
    provider.getService<SceneManager>()->initialize(&provider);
    provider.getService<ActionRegistry>()->initialize(&provider);
    provider.getService<NativeViewRegistry>()->initialize(&provider);
    provider.getService<IFIleLoader>()->initialize(&provider);
    provider.getService<ScriptManager>()->initialize(&provider);

    auto *sceneManager = provider.getService<SceneManager>();
    auto *inputManager = provider.getService<InputManager>();
    auto *actionRegistry = provider.getService<ActionRegistry>();
    auto *scriptManager = provider.getService<ScriptManager>();
    ASSERT_NE(sceneManager, nullptr);
    ASSERT_NE(inputManager, nullptr);
    ASSERT_NE(actionRegistry, nullptr);
    ASSERT_NE(scriptManager, nullptr);

    const uint64_t sceneGraphId = sceneManager->createSceneGraph("MouseDispatchTest");
    ASSERT_TRUE(sceneManager->createRoot(sceneGraphId));
    SceneGraph *graph = sceneManager->getSceneGraph(sceneGraphId);
    ASSERT_NE(graph, nullptr);
    Element *root = graph->getRoot();
    ASSERT_NE(root, nullptr);

    Element *bottom = sceneManager->createElement("div");
    Element *top = sceneManager->createElement("div");
    ASSERT_NE(bottom, nullptr);
    ASSERT_NE(top, nullptr);

    sceneManager->AppendElement(root->getUid(), bottom->getUid());
    sceneManager->AppendElement(root->getUid(), top->getUid());
    sceneManager->updateAttribute(bottom->getUid(), "width", 100);
    sceneManager->updateAttribute(bottom->getUid(), "height", 100);
    sceneManager->updateAttribute(top->getUid(), "width", 100);
    sceneManager->updateAttribute(top->getUid(), "height", 100);
    sceneManager->updateAttribute(top->getUid(), "translateY", -100);
    YGNodeStyleSetWidth(root->getLayoutNode(), 200.0f);
    YGNodeStyleSetHeight(root->getLayoutNode(), 200.0f);
    YGNodeCalculateLayout(root->getLayoutNode(), 200.0f, 200.0f, YGDirectionLTR);

    int clickCount = 0;
    std::string payload;
    actionRegistry->registerAction("record.click", [&](const ActionRequest &request) {
        ++clickCount;
        payload = request.payloadJson;
        return ActionResult::Success();
    });

    std::ostringstream script;
    script << "MachiNative.updateEventHandler(" << root->getUid()
           << "n, 'onClick', function(event) {"
           << "MachiNative.invokeAction('record.click', JSON.stringify({"
           << "type: event.type,"
           << "target: String(event.target),"
           << "currentTarget: String(event.currentTarget)"
           << "}));"
           << "});";
    scriptManager->Execute(script.str());

    MouseEvent down;
    down.type = MouseEvent::Type::Down;
    down.x = 20.0f;
    down.y = 20.0f;
    down.button = MouseButton::Left;
    down.isPressed = true;
    down.buttons = 1;
    inputManager->emitMouseEvent(down);

    MouseEvent move = down;
    move.type = MouseEvent::Type::Move;
    move.x = 23.0f;
    move.y = 23.0f;
    inputManager->emitMouseEvent(move);

    MouseEvent up = down;
    up.type = MouseEvent::Type::Up;
    up.x = 23.0f;
    up.y = 23.0f;
    up.isPressed = false;
    up.buttons = 0;
    inputManager->emitMouseEvent(up);

    EXPECT_EQ(clickCount, 1);
    EXPECT_NE(payload.find("\"type\":\"click\""), std::string::npos);
    EXPECT_NE(payload.find("\"target\":\"" + std::to_string(top->getUid()) + "\""), std::string::npos);
    EXPECT_NE(payload.find("\"currentTarget\":\"" + std::to_string(root->getUid()) + "\""), std::string::npos);
}
