SET(ALL_LIBS workers ed disruption_api calendar_api zmq time_tables types autocomplete proximitylist
    ptreferential time_tables data routing fare georef utils SimpleAmqpClient
    rabbitmq-static log4cplus tcmalloc ${Boost_LIBRARIES})

# every tests will produce an executable used in jormungandr integration tests

add_executable(main_routing_test main_routing_test.cpp)

add_executable(main_ptref_test main_ptref_test.cpp)

add_executable(basic_routing_test basic_routing_test.cpp)

add_executable(departure_board_test departure_board_test.cpp)

target_link_libraries(main_routing_test ${ALL_LIBS})
target_link_libraries(main_ptref_test ${ALL_LIBS})
target_link_libraries(basic_routing_test ${ALL_LIBS})
target_link_libraries(departure_board_test ${ALL_LIBS})
