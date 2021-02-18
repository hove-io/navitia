
FILE(GLOB ROUTING_SRC "*.cpp")

add_library(routing ${ROUTING_SRC})
target_link_libraries(routing  georef autocomplete pthread)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark data boost_program_options)

add_executable(benchmark_raptor_cache benchmark_raptor_cache.cpp)
target_link_libraries(benchmark_raptor_cache boost_program_options data profiler)

add_executable(benchmark_full benchmark_full.cpp)
target_link_libraries(benchmark_full boost_program_options data)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
