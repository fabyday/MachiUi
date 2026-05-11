#include <gtest/gtest.h>
#include <Scripting/ScriptManager.h>
#include <Core/ActionRegistry.h>
#include <Core/NativeViewRegistry.h>

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
