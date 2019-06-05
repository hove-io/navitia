add_library(apply_disruption apply_disruption.cpp)
add_dependencies(apply_disruption protobuf_files)
target_link_libraries(apply_disruption data pb_lib protobuf)

add_library(make_disruption_from_chaos make_disruption_from_chaos.cpp)
add_dependencies(make_disruption_from_chaos protobuf_files)
target_link_libraries(make_disruption_from_chaos apply_disruption data pb_lib protobuf)

add_library(rt_handling realtime.cpp)
add_dependencies(rt_handling protobuf_files)
target_link_libraries(rt_handling data pb_lib protobuf)

add_library(workers worker.cpp maintenance_worker.cpp configuration.cpp metrics.cpp)
add_dependencies(workers protobuf_files)
target_link_libraries(workers apply_disruption make_disruption_from_chaos rt_handling ${PQXX_LIB}
  SimpleAmqpClient equipment_api disruption_api calendar_api ptreferential autocomplete georef
  routing time_tables prometheus-cpp-core prometheus-cpp-pull)

add_library(fill_disruption_from_database fill_disruption_from_database.cpp)
target_link_libraries(fill_disruption_from_database make_disruption_from_chaos data types pb_lib
  ${PQXX_LIB} ${Boost_SERIALIZATION_LIBRARY} ${Boost_FORMAT_LIBRARY} protobuf)

add_executable(kraken kraken_zmq.cpp)
target_link_libraries(kraken workers types proximitylist
    ptreferential time_tables data pb_lib routing fare utils SimpleAmqpClient
    rabbitmq-static log4cplus ${Boost_THREAD_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_CHRONO_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} protobuf
    ${NAVITIA_ALLOCATOR})
add_dependencies(kraken protobuf_files)

install(TARGETS kraken DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
add_subdirectory(tests)
