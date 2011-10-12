FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} 
                      ${CMAKE_SOURCE_DIR}
)

SET(SOURCE
    tests.cpp
)


add_executable(navimake_test ${SOURCE})
target_link_libraries(navimake_test ${Boost_LIBRARIES} connectors)

ADD_TEST(navimake_test ${EXECUTABLE_OUTPUT_PATH}/navimake_test --report_level=no)
