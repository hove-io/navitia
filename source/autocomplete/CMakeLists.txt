
add_library(autocomplete autocomplete.cpp autocomplete_api.cpp)
target_link_libraries(autocomplete pb_lib)

SET(BOOST_LIBS ${BOOST_LIB} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY})
add_executable(autocomplete_test tests/test.cpp)
target_link_libraries(autocomplete_test georef data autocomplete pb_lib types thermometer fare routing ed utils ${BOOST_LIBS})
ADD_BOOST_TEST(autocomplete_test)
