FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework thread date_time serialization REQUIRED)
FIND_PACKAGE(WS_Commons)

MAKE_WS(navitia ZMQ navitia_ws.cpp LIBS types firstletter proximitylist georef ptreferential interface time_tables data routing utils ${Boost_LIBRARIES})

add_executable(navitia_zmq navitia_zmq.cpp worker.cpp)
target_link_libraries(navitia_zmq zmq time_tables types firstletter proximitylist ptreferential interface time_tables data routing georef utils ${Boost_LIBRARIES})

INSTALL_TARGETS(/navitia navitia)
