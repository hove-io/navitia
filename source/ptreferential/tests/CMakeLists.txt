FIND_PACKAGE(Boost 1.44 COMPONENTS unit_test_framework date_time thread regex serialization filesystem system REQUIRED)

add_executable(ptref_test ptret_test.cpp)
target_link_libraries(ptref_test ed ptreferential data routing types pb_lib georef utils autocomplete ${Boost_LIBRARIES} log4cplus)
ADD_TEST(ptref_test ${EXECUTABLE_OUTPUT_PATH}/ptref_test --report_level=no)
