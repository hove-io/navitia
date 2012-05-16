FIND_PACKAGE(Boost 1.40.0 COMPONENTS graph serialization unit_test_framework REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS})

add_library(streetnetwork types.cpp ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c)
target_link_libraries(streetnetwork ${Boost_LIBRARIES})

add_executable (test_sn test.cpp)
target_link_libraries(test_sn ${Boost_LIBRARIES} streetnetwork proj)
ADD_TEST(test_sn ${EXECUTABLE_OUTPUT_PATH}/test_sn --report_level=no)
