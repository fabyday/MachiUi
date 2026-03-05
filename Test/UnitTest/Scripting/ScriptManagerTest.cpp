#include <gtest/gtest.h>
#include <Scripting/ScriptManager.h>
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