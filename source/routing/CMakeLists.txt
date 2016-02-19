SET(BOOST_LIBS
  ${Boost_THREAD_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_REGEX_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SERIALIZATION_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})

SET(ROUTING_SRC
  routing.cpp raptor_solution_reader.cpp raptor.cpp raptor_api.cpp
  next_stop_time.cpp dataraptor.cpp journey_pattern_container.cpp get_stop_times.cpp)

add_library(routing ${ROUTING_SRC})
target_link_libraries(routing types fare georef utils autocomplete ${BOOST_LIBS})

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark routing  boost_program_options data routing)

add_library(routing_cli_utils routing_cli_utils.cpp)
add_executable(standalone single_run.cpp)
target_link_libraries(standalone
  routing_cli_utils routing pb_lib boost_program_options data fare routing thermometer
  time_tables pb_lib georef utils autocomplete ${BOOST_LIBS} log4cplus pthread protobuf)

add_executable(cli routing_cli.cpp)
target_link_libraries(cli
  linenoise routing_cli_utils routing pb_lib boost_program_options data fare routing thermometer
  time_tables pb_lib georef utils autocomplete ${BOOST_LIBS} log4cplus pthread protobuf)

add_executable(benchmark_full benchmark_full.cpp)
target_link_libraries(benchmark_full
  routing  boost_program_options data fare routing georef utils autocomplete time_tables
  ${BOOST_LIBS} log4cplus pb_lib protobuf)

add_subdirectory(tests)
