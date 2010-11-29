FIND_PACKAGE(Boost 1.40.0 COMPONENTS thread REQUIRED)

include_directories (${Boost_INCLUDE_DIRS})
add_executable (benchs tests/benchs.cpp)
target_link_libraries(benchs boost_thread fcgi)

