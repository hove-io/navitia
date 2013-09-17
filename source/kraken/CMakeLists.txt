FIND_PACKAGE(Boost 1.48.0 COMPONENTS thread date_time serialization regex REQUIRED)

add_executable(kraken kraken_zmq.cpp worker.cpp maintenance_worker.cpp)
target_link_libraries(kraken zmq time_tables types proximitylist ptreferential time_tables data routing georef utils autocomplete ${Boost_LIBRARIES} log4cplus tcmalloc)

INSTALL_TARGETS(/usr/bin/ kraken)
