#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_executable(data_manager_test data_manager_test.cpp)
target_link_libraries(data_manager_test workers utils log4cplus tcmalloc ${Boost_LIBRARIES})
ADD_BOOST_TEST(data_manager_test)
