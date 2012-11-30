SET(TIME_TABLES_SRC get_stop_times.cpp next_departures.cpp departure_board.cpp thermometer.cpp line_schedule.cpp)
add_library(time_tables ${TIME_TABLES_SRC})

add_subdirectory(tests)
