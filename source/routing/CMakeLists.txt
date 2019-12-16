
SET(ROUTING_SRC
  routing.cpp raptor_solution_reader.cpp raptor.cpp raptor_api.cpp
  next_stop_time.cpp dataraptor.cpp journey_pattern_container.cpp get_stop_times.cpp
  isochrone.cpp heat_map.cpp
  journey.cpp)

add_library(routing ${ROUTING_SRC})
target_link_libraries(routing  georef autocomplete pthread)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark data boost_program_options)

add_executable(benchmark_raptor_cache benchmark_raptor_cache.cpp)
target_link_libraries(benchmark_raptor_cache boost_program_options data profiler)

add_executable(benchmark_full benchmark_full.cpp)
target_link_libraries(benchmark_full boost_program_options data)

add_subdirectory(tests)
