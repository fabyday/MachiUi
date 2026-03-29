
file(GLOB CoreSource "./Core/*.cpp")
file(GLOB RendererSource "./Renderer/*.cpp")
if(WIN32)
    file(GLOB DX12Source "./Renderer/Backend/DX12/*.cpp")
    list(APPEND RendererSource ${DX12Source})
endif()
message(STATUS "Final Renderer Sources: ${RendererSource}")

file(GLOB DependancyManagementSource "./DependencyManagement/*.cpp")
file(GLOB ScriptingSource "./Scripting/*.cpp")


set(PlatformDependentSource "")

if(WIN32)
    file(GLOB PlatformSource "./MinimalPlatform/Win/*.cpp")
elseif(APPLE)
    file(GLOB PlatformSource "./MinimalPlatform/OSX/*.cpp" "./MinimalPlatform/OSX/*.mm")
else()
    file(GLOB PlatformSource "./MinimalPlatform/*.cpp")
endif()

set(Source "")
if(${STANDALONE_MODE})
    set(Source ${PlatformSource} ${CoreSource} ${RendererSource} ${ScriptingSource})
else()
    set(Source ${CoreSource} ${RendererSource} ${ScriptingSource})
endif()


add_library(MachiUi_OBJECT_DEPS INTERFACE)
target_include_directories(MachiUi_OBJECT_DEPS INTERFACE ${PROJECT_SOURCE_DIR}/ThirdParty/yoga ${PROJECT_SOURCE_DIR}/ThirdParty/yoga ${PROJECT_SOURCE_DIR}/Source)
target_link_libraries(MachiUi_OBJECT_DEPS INTERFACE qjs yogacore spdlog::spdlog ${OS_RELATED_LIBS} MachiUi_OBJECT_DEPS boost_di)


add_library(MachiUi_OBJECT OBJECT ${Source})
target_link_libraries(MachiUi_OBJECT PRIVATE MachiUi_OBJECT_DEPS)

# add_library(MachiUi_TEST_OBJECT OBJECT ${Source})
# target_compile_definitions(MachiUi_TEST_OBJECT PRIVATE MACHI_UI_TEST)
# target_link_libraries(MachiUi_TEST_OBJECT PRIVATE MachiUi_OBJECT_DEPS)



# target_link_libraries(MachiUi_OBJECT PRIVATE qjs yogacore spdlog::spdlog ${OS_RELATED_LIBS})
# target_include_directories(MachiUi_OBJECT PRIVATE ${PROJECT_SOURCE_DIR}/ThirdParty/yoga ${PROJECT_SOURCE_DIR}/Source)

# # 



add_library(MachiUi STATIC $<TARGET_OBJECTS:MachiUi_OBJECT>)
target_include_directories(MachiUi INTERFACE ${PROJECT_SOURCE_DIR}/Source
)
target_link_libraries(MachiUi PRIVATE MachiUi_OBJECT_DEPS)


if(MSVC)
    target_link_options(MachiUi INTERFACE /WHOLEARCHIVE:$<TARGET_FILE:MachiUi>)
elseif(APPLE)
    # 맥 (Clang/LD) 설정
    target_link_options(
        MachiUi INTERFACE "-Wl,-force_load,$<TARGET_FILE:MachiUi>")
else()
    # 리눅스 (GCC/Gold) 설정
    target_link_options(MachiUi
        PUBLIC "-Wl,--whole-archive" MachiUi "-Wl,--no-whole-archive")
endif()
