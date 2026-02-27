project(MachiUI)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



option(STANDALONE_MODE "stasndalone embeded" TRUE)


# set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(/utf-8)
add_subdirectory(ThirdParty/yoga/yoga)
add_subdirectory(ThirdParty/quickjs)
add_subdirectory(ThirdParty/spdlog)

file(GLOB_RECURSE MachiUi_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/Source/**/*.h)
file(GLOB_RECURSE MachiUi_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Source/**/*.cpp)


add_library(MachiUi STATIC ${MachiUi_SOURCES} ${MachiUi_HEADERS})
target_link_options(MachiUi INTERFACE /WHOLEARCHIVE:$<TARGET_FILE:MachiUi>)

target_link_libraries(MachiUi PRIVATE qjs yogacore spdlog::spdlog)
target_include_directories(MachiUi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
message("${CMAKE_CURRENT_SOURCE_DIR}/Source")
# add_executable(test WIN32 examples/main.cpp)
add_executable(test2 WIN32 examples/main_windowmanager.cpp)
# target_link_libraries(test PRIVATE MachiUi)
target_link_libraries(test2 PRIVATE MachiUi)
# set_target_properties(test PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
set_target_properties(test2 PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
# set_target_properties(test2 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")


set(SOURCE_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(DEST_ASSETS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

# 만약 디렉토리 출력을 따로 지정 안 했다면 보통 이 경로입니다.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(DEST_ASSETS "${CMAKE_CURRENT_BINARY_DIR}/assets")
endif()

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

