add_library(thermometer thermometer.cpp)
target_link_libraries(thermometer types)    

SET(TIME_TABLES_SRC passages.cpp route_schedules.cpp departure_boards.cpp request_handle.cpp)
add_library(time_tables ${TIME_TABLES_SRC})
#TODO: a static lib doesn't need to be linked with is dependency
target_link_libraries(time_tables utils types routing autocomplete proximitylist ptreferential georef thermometer)


add_subdirectory(tests)


