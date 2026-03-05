project(MachiUI)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_TEST ON)


option(STANDALONE_MODE "stasndalone embeded" TRUE)

set(OS_RELATED_LIBS "")
if(MSVC)
    add_compile_options(/utf-8)
else()
    # clang, gcc don't need utf-8 flag


endif()

add_subdirectory(ThirdParty/yoga/yoga)
add_subdirectory(ThirdParty/quickjs)
add_subdirectory(ThirdParty/spdlog)

file(GLOB_RECURSE MachiUi_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/Source/**/*.h)
file(GLOB_RECURSE MachiUi_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Source/**/*.cpp)
if(WIN32)
    message("이곳은 윈도우 환경입니다.")
elseif(APPLE)
message("APPLE")
set(OS_RELATED_LIBS "-framework AppKit" "-framework Metal" "-framework QuartzCore")
elseif(UNIX)
    message("이곳은 리눅스 또는 기타 유닉스 환경입니다.")
endif()
message("test232")
    
message(STATUS "Current OS_RELATED_LIBS: ${OS_RELATED_LIBS}")
message("${OS_RELATED_LIBS}")
add_library(MachiUi STATIC ${MachiUi_SOURCES} ${MachiUi_HEADERS})
target_link_options(MachiUi INTERFACE /WHOLEARCHIVE:$<TARGET_FILE:MachiUi>)

target_link_libraries(MachiUi PRIVATE qjs yogacore spdlog::spdlog ${OS_RELATED_LIBS})
target_include_directories(MachiUi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
message("${CMAKE_CURRENT_SOURCE_DIR}/Source")
# add_executable(test WIN32 examples/main.cpp)


if(WIN32)
    add_executable(test2 WIN32 examples/main_windowmanager.cpp)
    message("이곳은 윈도우 환경입니다.")
elseif(APPLE)
    add_executable(test2 examples/main_windowmanager.cpp)
elseif(UNIX)
    message("이곳은 리눅스 또는 기타 유닉스 환경입니다.")
endif()

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


if(BUILD_TEST)
message("build test...")
    add_subdirectory(Test)
endif()

