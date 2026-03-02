include(FetchContent)

# 1. Google Test 다운로드 및 로드
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: gtest 빌드 시 런타임 라이브러리(/MT, /MD) 충돌 방지
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 2. 테스트 실행 파일 생성
add_executable(MachiUiTests
    ScriptManagerTest.cpp
    # 필요한 경우 다른 테스트 파일 추가
)

# 3. 링크 설정
# - gtest_main: main() 함수를 직접 안 짜도 gtest가 제공해줍니다.
# - MachiCore: 테스트할 대상 라이브러리
target_link_libraries(MachiUiTests
    GTest::gtest_main
     MachiUi
)

# 4. VS Code의 Test Explorer가 테스트를 인식하도록 설정
include(GoogleTest)
gtest_discover_tests(MachiUiTests)