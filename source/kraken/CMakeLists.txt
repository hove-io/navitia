add_library(apply_disruption apply_disruption.cpp)
target_link_libraries(apply_disruption types config)

add_library(make_disruption_from_chaos make_disruption_from_chaos.cpp)
target_link_libraries(make_disruption_from_chaos apply_disruption )

add_library(rt_handling realtime.cpp)
target_link_libraries(rt_handling apply_disruption )

add_library(workers worker.cpp maintenance_worker.cpp configuration.cpp metrics.cpp)
target_link_libraries(workers
    rt_handling
    SimpleAmqpClient
    equipment_api
    position_api
    access_point_api
    disruption_api
    calendar_api
    time_tables
    z
    prometheus-cpp-pull
    prometheus-cpp-core
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

add_library(fill_disruption_from_database fill_disruption_from_database.cpp)
target_link_libraries(fill_disruption_from_database
    make_disruption_from_chaos
    data
    types
    pb_lib
    ${PQXX_LIB}
    ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_FORMAT_LIBRARY}
)

add_executable(kraken kraken_zmq.cpp)
target_link_libraries(kraken workers ${NAVITIA_ALLOCATOR} ${Boost_THREAD_LIBRARY})
add_dependencies(kraken protobuf_files prometheus-cpp::pull)

install(TARGETS kraken DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
