#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# every tests will produce an executable used in jormungandr integration tests

add_executable(main_routing_test main_routing_test.cpp)
target_link_libraries(main_routing_test workers ed disruption_api calendar_api zmq time_tables types autocomplete proximitylist
    ptreferential time_tables data routing fare georef utils SimpleAmqpClient
    rabbitmq-static log4cplus tcmalloc ${Boost_LIBRARIES})
