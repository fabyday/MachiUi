function(copyTarget sourceTarget destTarget)
    # 1. 원본 타겟의 소스 파일 목록 가져오기
    get_target_property(SRCS ${sourceTarget} SOURCES)

    # 2. 새 타겟 생성 (원본과 동일하게 STATIC/OBJECT 등 지정 가능)
    add_library(${destTarget} STATIC ${SRCS})
    
    # 3. 인클루드 디렉토리 복사
    get_target_property(INCS ${sourceTarget} INCLUDE_DIRECTORIES)
    if(INCS)
        target_include_directories(${destTarget} PUBLIC ${INCS})
    endif()

    # 4. 컴파일 정의(Macro) 복사
    get_target_property(DEFS ${sourceTarget} COMPILE_DEFINITIONS)
    if(DEFS)
        target_compile_definitions(${destTarget} PRIVATE ${DEFS})
    endif()

    # 5. 링크 라이브러리(의존성) 복사
    get_target_property(LIBS ${sourceTarget} LINK_LIBRARIES)
    if(LIBS)
        target_link_libraries(${destTarget} PRIVATE ${LIBS})
    endif()
endfunction()

# --- 사용 예시 ---
# MachiUi_OBJECT를 복사해서 테스트 전용 라이브러리 생성
copy_target_settings(MachiUi_OBJECT MACHIUI_TEST_LIB)

# 복사본에만 테스트용 매크로 추가
target_compile_definitions(MACHIUI_TEST_LIB PRIVATE MACHI_UI_TEST)
