include(FetchContent)

# 1. Google Test 다운로드 및 로드
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: gtest 빌드 시 런타임 라이브러리(/MT, /MD) 충돌 방지
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# helper for creating google-test targets
include(${PROJECT_SOURCE_DIR}/cmakes/test.cmake)
add_subdirectory(UnitTest)
# 2. 테스트 실행 파일 생성
# call helper from cmakes/test.cmake; additional libs can be listed after the
# source list (MachiUi in this case).

