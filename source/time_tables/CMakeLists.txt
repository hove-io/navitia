SET(TIME_TABLES_SRC next_departures.cpp departure_board.cpp)
add_library(time_tables ${TIME_TABLES_SRC})
target_link_libraries(time_tables types data ptreferential routing)
