add_library(apply_disruption apply_disruption.cpp)
target_link_libraries(apply_disruption data pb_lib protobuf)

add_library(make_disruption_from_chaos make_disruption_from_chaos.cpp)
target_link_libraries(make_disruption_from_chaos apply_disruption data pb_lib protobuf)

add_library(rt_handling realtime.cpp)
target_link_libraries(rt_handling data pb_lib protobuf)

# Add Prometheus
if (PROMETHEUS_IS_ACTIVED)
  set(PROMETHEUS_SOURCES metrics.cpp)
  message("-- Add Prometheus sources in kraken")
else (PROMETHEUS_IS_ACTIVED)
  message("-- Skip Prometheus sources in kraken")
endif (PROMETHEUS_IS_ACTIVED)

add_library(workers worker.cpp maintenance_worker.cpp configuration.cpp ${PROMETHEUS_SOURCES})
target_link_libraries(workers apply_disruption make_disruption_from_chaos rt_handling ${PQXX_LIB}
  SimpleAmqpClient disruption_api calendar_api ptreferential autocomplete georef
  routing time_tables tcmalloc)

if (PROMETHEUS_IS_ACTIVED)
  target_link_libraries(workers prometheus-cpp)
endif (PROMETHEUS_IS_ACTIVED)

add_library(fill_disruption_from_database fill_disruption_from_database.cpp)
target_link_libraries(fill_disruption_from_database make_disruption_from_chaos data types pb_lib
  ${PQXX_LIB} ${Boost_SERIALIZATION_LIBRARY} ${Boost_FORMAT_LIBRARY} protobuf)

add_executable(kraken kraken_zmq.cpp)
target_link_libraries(kraken workers types proximitylist
    ptreferential time_tables data pb_lib routing fare utils SimpleAmqpClient
    rabbitmq-static log4cplus ${Boost_THREAD_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_CHRONO_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} protobuf)

INSTALL_TARGETS(/usr/bin/ kraken)
add_subdirectory(tests)
