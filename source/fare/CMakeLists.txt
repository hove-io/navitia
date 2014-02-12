SET(GEOREF_SRC
    fare.h
    fare.cpp
)

add_library(fare ${GEOREF_SRC})

add_subdirectory(tests)
