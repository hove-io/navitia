
add_library(workers worker.cpp maintenance_worker.cpp)

add_executable(kraken kraken_zmq.cpp)
target_link_libraries(kraken workers disruption_api calendar_api zmq time_tables types autocomplete proximitylist
    ptreferential time_tables data pb_lib routing fare georef utils SimpleAmqpClient
    rabbitmq-static log4cplus tcmalloc ${Boost_THREAD_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_CHRONO_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})

INSTALL_TARGETS(/usr/bin/ kraken)
add_subdirectory(tests)
