#include <gtest/gtest.h>
#include <Core/Element.h>







TEST(ElementTest, Element_Get_Set) {
    // ScriptManager sm;
    auto* element = new Element(1000u);    
    const char * str = "test is fine";
    element->setText(str);
    EXPECT_EQ(str, element->getText());
}
