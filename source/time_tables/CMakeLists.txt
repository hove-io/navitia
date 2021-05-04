add_library(thermometer thermometer.cpp)
add_dependencies(thermometer protobuf_files)

SET(TIME_TABLES_SRC passages.cpp route_schedules.cpp departure_boards.cpp request_handle.cpp)
add_library(time_tables ${TIME_TABLES_SRC})
target_link_libraries(time_tables routing thermometer types)
add_dependencies(time_tables protobuf_files)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
