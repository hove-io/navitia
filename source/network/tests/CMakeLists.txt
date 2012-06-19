FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )

SET(SOURCE
    time_dep_test.cpp
)

add_executable(time_dep_test ${SOURCE})
target_link_libraries(time_dep_test ${Boost_LIBRARIES} network connectors)

ADD_TEST(time_dep_test ${EXECUTABLE_OUTPUT_PATH}/time_dep_test --report_level=no)
