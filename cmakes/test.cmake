# helper macros for test targets
#
# addTest(<name> <source-files> [<dependency-lib>...])
#
# Creates a GoogleTest executable, links it with the provided dependencies
# (for example your library target), adds the project include directory so
# that headers can be referenced as <Core/header.h>, and registers the
# target with gtest_discover_tests for integration with IDE test explorers.
#
# The signature mirrors the way CMake handles lists: any arguments after the
# source list are treated as dependency libraries.  Example usage:
#
#   # file paths are relative to the CMakeLists.txt where addTest is
#   # invoked.  If your sources live in subdirectories, include that part
#   # of the path (e.g. "Scripting/ScriptManagerTest.cpp").  Quotes are not
#   # required unless you need to group multiple files with a semicolon or
#   # there are spaces in the path.
#   addTest(MachiUiTests
#       Scripting/ScriptManagerTest.cpp
#       MachiUi
#   )
#
# When multiple dependency libraries are needed simply separate them with
# spaces.  They will be passed through to target_link_libraries.

function(addTest targetName sources engineObject)
    # sources may be a semicolon‑separated list; preserve it as-is.
    add_executable(${targetName} ${sources})

    message("ADD TEST : ${targetName} ${sources} engine ${engineObject} args ${ARGN}")
    # link with gtest and any additional libraries passed after the sources
    target_link_libraries(${targetName} PRIVATE GTest::gtest_main GTest::gmock ${engineObject} ${ARGN})
    # target_link_libraries(${targetName} PRIVATE GTest::gtest_main ${ARGN})
    # ensure tests can #include <Core/...> etc. by pointing to project Source
    target_include_directories(${targetName} PRIVATE
        ${PROJECT_SOURCE_DIR}/Source
    )

    # register with GoogleTest so that IDE explorers pick it up
    include(GoogleTest)
    gtest_discover_tests(${targetName})
endfunction()
