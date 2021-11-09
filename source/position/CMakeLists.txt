add_library(position_api position_api.cpp)
target_link_libraries(position_api pb_lib)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
