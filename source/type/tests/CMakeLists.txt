FIND_PACKAGE(Boost 1.42.0 COMPONENTS serialization date_time regex unit_test_framework REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )
 
add_executable (indexes_tests simple_test.cpp fixtures.h)
target_link_libraries(indexes_tests ${Boost_LIBRARIES})

ADD_TEST(test_indexes ${EXECUTABLE_OUTPUT_PATH}/indexes_tests --report_level=no)

