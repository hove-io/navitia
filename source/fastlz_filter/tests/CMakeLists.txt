FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework iostreams REQUIRED)
 
add_executable (fastlz_tests test.cpp ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c)
target_link_libraries(fastlz_tests ${Boost_LIBRARIES})

ADD_TEST(test_fastlzfilter ${EXECUTABLE_OUTPUT_PATH}/fastlz_tests --report_level=no)

