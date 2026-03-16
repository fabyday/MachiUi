# add_library(MachiUi_TEST_OBJECT OBJECT ${Source})
# target_compile_definitions(MachiUi_TEST_OBJECT PRIVATE MACHI_UI_TEST)
# target_link_libraries(MachiUi_TEST_OBJECT PRIVATE MachiUi_OBJECT_DEPS)

get_target_property(MACHI_UI_SOURCES MachiUi_OBJECT SOURCES)
add_library(MACHIUI_TEST_LIB STATIC ${MACHI_UI_SOURCES})
target_compile_definitions(MACHIUI_TEST_LIB PRIVATE MACHI_UI_TEST)
target_link_libraries(MACHIUI_TEST_LIB PUBLIC MachiUi_OBJECT_DEPS)




#Core
addTest(ElementTest Core/ElementTest.cpp
    MACHIUI_TEST_LIB
    
)

#Core
addTest(ViewManagerTest Core/ViewManagerTest.cpp
    MACHIUI_TEST_LIB
)


addTest(DefaultTimerTest Core/DefaultTimerTest.cpp
    MACHIUI_TEST_LIB
)

# graphics backend test
if(WIN32)
addTest(DX12BackendTest
    Renderer/DX12/BackendTest.cpp
    MACHIUI_TEST_LIB
)
endif()

# Scrpiting
addTest(MachiUiTests
    Scripting/ScriptManagerTest.cpp
    MACHIUI_TEST_LIB
)

