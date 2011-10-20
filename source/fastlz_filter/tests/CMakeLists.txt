FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework iostreams REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} ../ ${CMAKE_SOURCE_DIR}/../third_party/fastlz)
 
add_executable (fastlz_tests test.cpp ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c)
target_link_libraries(fastlz_tests ${Boost_LIBRARIES})

ADD_TEST(test_fastlzfilter ${EXECUTABLE_OUTPUT_PATH}/fastlz_tests --report_level=no)

