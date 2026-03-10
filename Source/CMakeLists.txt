
file(GLOB CoreSource "./Core/*.cpp")
file(GLOB RendererSource "./Renderer/*.cpp")
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

add_library(MachiUi STATIC ${Source})

target_link_libraries(MachiUi PRIVATE qjs yogacore spdlog::spdlog ${OS_RELATED_LIBS})

if(MSVC)
    target_link_options(MachiUi INTERFACE /WHOLEARCHIVE:$<TARGET_FILE:MachiUi>)
elseif(APPLE)
    # 맥 (Clang/LD) 설정
    target_link_options(
        MachiUi INTERFACE "-Wl,-force_load,$<TARGET_FILE:MachiUi>")
else()
    # 리눅스 (GCC/Gold) 설정
    target_link_options(MachiUi
        PRIVATE "-Wl,--whole-archive" MachiUi "-Wl,--no-whole-archive")
endif()

target_include_directories(MachiUi PUBLIC ${PROJECT_SOURCE_DIR}/ThirdParty/yoga ${PROJECT_SOURCE_DIR}/Source)
