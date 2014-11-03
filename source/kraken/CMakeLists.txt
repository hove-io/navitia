
add_library(workers worker.cpp maintenance_worker.cpp configuration.cpp fill_disruption_from_chaos.cpp)
target_link_libraries(workers disruption_api calendar_api ptreferential autocomplete georef
  routing time_tables tcmalloc)

add_executable(kraken kraken_zmq.cpp)
target_link_libraries(kraken workers zmq types proximitylist
    ptreferential time_tables data pb_lib routing fare utils SimpleAmqpClient
    rabbitmq-static log4cplus ${Boost_THREAD_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_CHRONO_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} protobuf)

INSTALL_TARGETS(/usr/bin/ kraken)
add_subdirectory(tests)
