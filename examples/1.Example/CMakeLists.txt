add_machiUi_execuable(MachiUiExample1 main.cpp CPPBack/Callback.cpp)
option(EXAMPLE1_MANIFEST_MODE "Use the 1.Example manifest" ON)
if(EXAMPLE1_MANIFEST_MODE)
    target_compile_definitions(MachiUiExample1 PRIVATE MANIFEST_FILE_MODE
        MACHIUI_EXAMPLE_MANIFEST="${CMAKE_CURRENT_BINARY_DIR}/manifest.json")
endif()
configure_file(manifest.json manifest.json COPYONLY)
