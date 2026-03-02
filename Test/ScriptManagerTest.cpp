#include <gtest/gtest.h>
#include "../Source/Scripting/ScriptManager.h" // 경로 주의

TEST(ScriptManagerTest, ContextStackPushPop) {
    // ScriptManager sm;
    ScriptExecutionContext ctx;
    
    // sm.pushContext(&ctx);
    // EXPECT_EQ(sm.getActiveContext(), &ctx);
    EXPECT_EQ(10, 10);
    // sm.popContext();
    // EXPECT_EQ(sm.getActiveContext(), nullptr);
}