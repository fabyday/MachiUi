set(Sources)

file(GLOB Core "./Core/*.cpp")
file(GLOB Renderer "./Renderer/*.cpp")
file(GLOB Scripting "./Scripting/*.cpp")

set(PlatformSource "")
if(WIN32)
file(GLOB PlatformSource "./MinimalPlatform/*.cpp")
elseif(APPLE)
file(GLOB PlatformSource "./MinimalPlatform/OSX/*.cpp")
else()
file(GLOB PlatformSource "./MinimalPlatform/*.cpp")
endif()




add_library(MachiUI STATIC ${PlatformSource} ${Core} ${Renderer} ${Scripting})
target_link_libraries(MachiUi PRIVATE qjs yogacore spdlog::spdlog ${OS_RELATED_LIBS})
target_link_options(MachiUi INTERFACE /WHOLEARCHIVE:$<TARGET_FILE:MachiUi>)
target_include_directories(MachiUi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
