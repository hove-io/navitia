FIND_PACKAGE(Boost 1.48.0 COMPONENTS unit_test_framework thread date_time serialization REQUIRED)

add_executable(navitia navitia_zmq.cpp worker.cpp maintenance_worker.cpp)
target_link_libraries(navitia zmq time_tables types autocomplete proximitylist ptreferential time_tables data routing georef utils ${Boost_LIBRARIES})

INSTALL_TARGETS(/usr/bin/ navitia)
