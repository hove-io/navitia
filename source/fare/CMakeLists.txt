SET(FARE_SRC
    fare.h
    fare.cpp
    fare_api.cpp
)

add_library(fare ${FARE_SRC})
target_link_libraries(fare routing pb_lib)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
