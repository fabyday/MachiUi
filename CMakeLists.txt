set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(MachiUI)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BUILD_TEST "Build GoogleTest unit tests" ON)
option(BUILD_METAL_SMOKE "Build standalone Metal smoke example" ON)
option(STANDALONE_MODE "stasndalone embeded" TRUE)
option(DEFAULT_LOGGER "stasndalone embeded" TRUE)

set(OS_RELATED_LIBS "")
if(MSVC)
    add_compile_options(/utf-8)
else()
    # clang, gcc don't need utf-8 flag


endif()
include("./cmakes/helper.cmake")
# DEPS
add_subdirectory(ThirdParty/yoga/yoga)
add_subdirectory(ThirdParty/quickjs)
if(${DEFAULT_LOGGER})
    add_subdirectory(ThirdParty/spdlog)
endif()

# boost DI SINGLE HEADER
add_library(boost_di INTERFACE)
target_include_directories(boost_di INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/boost-di")



if(WIN32)
message("Platform : Windows")
elseif(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    message("Platform : Apple")
    set(OS_RELATED_LIBS "-framework AppKit" "-framework Metal" "-framework QuartzCore")
elseif(UNIX)
message("Platform : Linux-UnixLike")
endif()

add_subdirectory(Source)

# add_executable(test WIN32 examples/main.cpp)



# target_link_libraries(test PRIVATE MachiUi)
# set_target_properties(test PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
# set_target_properties(test2 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

add_machiUi_execuable(test2 "examples/main_windowmanager.cpp")
add_subdirectory(examples/1.Example)
if(WIN32)
set_target_properties(test2 PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
endif()



set(SOURCE_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
set(DEST_ASSETS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(DEST_ASSETS "${CMAKE_CURRENT_BINARY_DIR}/assets")
endif()
moveAssetToTargetDir(test2 "assets" ${SOURCE_ASSETS})
# 2. 빌드 후 이벤트로 심볼릭 링크(또는 정션) 생성
add_custom_command(TARGET test2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Target directory: $<TARGET_FILE_DIR:test2>"
    # 실행 파일이 있는 바로 그 폴더($<TARGET_FILE_DIR:test2>) 내부에 assets 링크 생성
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:test2>/assets"

    # Windows와 Unix 호환성을 위해 분기 처리 (권장)
    COMMAND ${CMAKE_COMMAND} -E create_symlink "${SOURCE_ASSETS}" "$<TARGET_FILE_DIR:test2>/assets"

    COMMENT "Linking assets to the actual executable directory"
    VERBATIM
)


set(JS_DIST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Source/Javascript/dist)
set(JS_DIST_DEST $<TARGET_FILE_DIR:test2>/assets/platform)




if(BUILD_TEST)
    enable_testing()
    message("build test...")
    add_subdirectory(Test)
endif()



if(APPLE AND BUILD_METAL_SMOKE)
    # Isolate the backend from unfinished engine bootstrap code.
    add_executable(MetalBackendSmoke ${PROJECT_SOURCE_DIR}/Test/UnitTest/Renderer/Metal/BackendSmoke.mm)
    target_include_directories(MetalBackendSmoke PRIVATE ${PROJECT_SOURCE_DIR}/Source)
    target_compile_options(MetalBackendSmoke PRIVATE -fobjc-arc)
    target_link_libraries(MetalBackendSmoke PRIVATE MachiUi ${OS_RELATED_LIBS})
    add_test(NAME MetalBackendSmoke COMMAND MetalBackendSmoke)
endif()
