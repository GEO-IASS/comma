SET(KIT string)

FILE( GLOB source ${SOURCE_CODE_BASE_DIR}/${KIT}/test/*test.cpp )

ADD_EXECUTABLE( ${CMAKE_PROJECT_NAME}_test_${KIT} ${source} )

TARGET_LINK_LIBRARIES( ${CMAKE_PROJECT_NAME}_test_${KIT} comma_${KIT} ${GTEST_BOTH_LIBRARIES} pthread )

IF( INSTALL_TESTS )
INSTALL ( 
    FILES ${PROJECT_BINARY_DIR}/bin/${CMAKE_PROJECT_NAME}_test_${KIT}
    PERMISSIONS WORLD_READ GROUP_READ OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    DESTINATION ${comma_CPP_TESTS_INSTALL_DIR} )
ENDIF( INSTALL_TESTS )

add_test( NAME ${CMAKE_PROJECT_NAME}_test_${KIT} COMMAND ${CMAKE_PROJECT_NAME}_test_${KIT} WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin )
