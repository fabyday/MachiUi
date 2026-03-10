project(MachiUI)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_TEST ON)


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

if(WIN32)
message("Platform : Windows")
elseif(APPLE)
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
if(WIN32)
set_target_properties(test2 PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
endif()



set(SOURCE_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
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

add_custom_command(TARGET test2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${JS_DIST_DEST}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${JS_DIST_SOURCE} ${JS_DIST_DEST}
)


if(BUILD_TEST)
    message("build test...")
    add_subdirectory(Test)
endif()

