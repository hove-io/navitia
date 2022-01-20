add_library(access_point_api access_point_api.cpp)
target_link_libraries(access_point_api pb_lib)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
