#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

SET(DATA_NAV_NAME "data.nav.lz4")

SET(DATA_DIR_TO_LOAD
    ${FIXTURES_DIR}/ed/ntfs/
)

#TODO clean up, it should be automatic (loop on DIR_TO_LOAD?)s
SET(DATA_NAV_TO_CREATE
    ${CMAKE_CURRENT_BINARY_DIR}/ntfs_${DATA_NAV_NAME}
)

SET(EITRI "${CMAKE_SOURCE_DIR}/eitri/eitri.py")

set($ENV{PYTHONPATH} "${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")
add_custom_command(OUTPUT ${DATA_NAV_TO_CREATE}
    COMMAND ${EITRI}
    ARGS "${FIXTURES_DIR}/ed/ntfs/" --output-file ${DATA_NAV_TO_CREATE} --ed-component-path "${CMAKE_BINARY_DIR}/ed" --add-pythonpath "${CMAKE_SOURCE_DIR}/navitiacommon" # todo for all DATA_NAV_TO_CREATE
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM
)

add_custom_target(datanav_files DEPENDS ${DATA_NAV_TO_CREATE})

set_source_files_properties(${DATA_NAV_TO_CREATE} PROPERTIES GENERATED TRUE)


add_executable(ed_integration_test ed_integration_tests.cpp)
target_link_libraries(ed_integration_test data georef types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(ed_integration_test)

SET_PROPERTY(TEST ed_integration_test APPEND PROPERTY ENVIRONMENT "INPUT_FILE_PATH=${DATA_NAV_TO_CREATE}")

add_dependencies(docker_test ed_integration_test)

add_dependencies(ed_integration_test datanav_files) # TODO add dependency to ed_integration_test
