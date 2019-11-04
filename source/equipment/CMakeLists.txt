add_library(equipment_api equipment_api.cpp)
target_link_libraries(equipment_api pb_lib)
add_subdirectory(tests)
