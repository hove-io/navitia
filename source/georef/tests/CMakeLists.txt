SET(GEOREF_SRC_TEST
    builder.h
    builder.cpp
)
add_library(georef_test_utils ${GEOREF_SRC_TEST})

add_executable(georef_test georef_test.cpp)
target_link_libraries(georef_test georef_test_utils georef utils  
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_THREAD_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_SERIALIZATION_LIBRARY} log4cplus)
ADD_TEST(georef_test ${EXECUTABLE_OUTPUT_PATH}/georef_test --report_level=no)

add_executable(street_network_test street_network_test.cpp)
target_link_libraries(street_network_test georef_test_utils georef data autocomplete utils 
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_THREAD_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_SERIALIZATION_LIBRARY} log4cplus)
ADD_TEST(street_network_test ${EXECUTABLE_OUTPUT_PATH}/street_network_test --report_level=no)
