SET(FARE_SRC
    fare.h
    fare.cpp
)

add_library(fare ${FARE_SRC})
target_link_libraries(fare routing pb_lib)
add_subdirectory(tests)
