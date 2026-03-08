function(add_machiUi_execuable targetName)
    if(WIN32)
        add_executable(${targetName} WIN32 ${ARGN})
    elseif(APPLE)
        add_executable(${targetName} ${ARGN})
    elseif(UNIX)
        add_executable(${targetName} ${ARGN})
    endif()
    target_link_libraries(${targetName} PRIVATE MachiUi)

endfunction()

function(moveAssetToTargetDir targetName targetPath assets)

    # 2. 빌드 후 이벤트로 심볼릭 링크(또는 정션) 생성
    add_custom_command(TARGET test2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Target directory: $<TARGET_FILE_DIR:${targetName}>"
        # 실행 파일이 있는 바로 그 폴더($<TARGET_FILE_DIR:test2>) 내부에 assets 링크 생성
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:test2>/${targetPath}"

        # Windows와 Unix 호환성을 위해 분기 처리 (권장)
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${assets}" "$<TARGET_FILE_DIR:${targetName}>/${targetPath}"

        COMMENT "Linking assets to the actual executable directory"
        VERBATIM
    )


endfunction()


# target_link_libraries(test PRIVATE MachiUi)
# set_target_properties(test PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
# set_target_properties(test2 PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
# set_target_properties(test2 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")


