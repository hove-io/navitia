#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

#add_executable(ed_integration_test ed_integration_tests.cpp)
#target_link_libraries(ed_integration_test ed types utils ${BOOST_LIBS} log4cplus)
#ADD_BOOST_TEST(ed_integration_test)
#add_dependencies(docker_test ed_integration_test)
