add_library(disruption_api traffic_reports_api.cpp line_reports_api.cpp)
target_link_libraries(disruption_api pb_lib)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
