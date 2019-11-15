SET(GEOREF_SRC
    fare.h
    fare.cpp
)

add_library(fare ${GEOREF_SRC})
target_link_libraries(fare pb_lib)
add_subdirectory(tests)
