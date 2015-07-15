#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

SET(DATA_NAV_NAME "data.nav.lz4")

SET(DATA_DIR_TO_LOAD
    ${FIXTURES_DIR}/ed/ntfs/
)

#TODO clean up it should be automatic
SET(DATA_NAV_TO_CREATE
    ${FIXTURES_DIR}/ed/ntfs/${DATA_NAV_NAME}
)

SET(EITRI "${CMAKE_SOURCE_DIR}/eitri/eitri.py")

SET_PROPERTY(TARGET eitri PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")

SET_PROPERTY(TARGET eitri APPEND PROPERTY
    ENVIRONMENT "ED_COMPONENT_PATH=${CMAKE_BINARY_DIR}/ed")

add_custom_command(OUTPUT ${DATA_NAV_TO_CREATE}
    COMMAND ${EITRI}
    ARGS ${FIXTURES_DIR}/ed/ntfs/${DATA_NAV_NAME} # todo for all DATA_NAV_TO_CREATE
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM
)

add_custom_target(datanav_files DEPENDS ${DATA_NAV_TO_CREATE})
set_source_files_properties(${DATA_NAV_TO_CREATE} PROPERTIES GENERATED TRUE)


add_executable(ed_integration_test ed_integration_tests.cpp)
target_link_libraries(ed_integration_test ed types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(ed_integration_test)

add_dependencies(docker_test ed_integration_test)

add_dependencies(ed_integration_test datanav_files) # TODO add dependency to ed_integration_test
