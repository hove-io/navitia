#FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)


SET(TIME_TABLES_SRC get_stop_times.cpp next_stop_times.cpp 2stops_schedule.cpp thermometer.cpp line_schedule.cpp departure_board.cpp parse_request.cpp)
add_library(time_tables ${TIME_TABLES_SRC})
#target_link_libraries(time_tables utils types routing autocomplete proximitylist ptreferential georef )


add_subdirectory(tests)


