add_library(equipment_api equipment_api.cpp)
target_link_libraries(equipment_api pb_lib)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
