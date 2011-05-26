FIND_PACKAGE(Boost 1.40.0 COMPONENTS graph unit_test_framework REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS})
 
add_executable (fare main.cpp fare.cpp fare.h)


add_executable (fare_tests fare.cpp fare.h test.cpp)
target_link_libraries(fare_tests ${Boost_LIBRARIES})
ADD_TEST(test_fare ${EXECUTABLE_OUTPUT_PATH}/fare_tests --report_level=no)

