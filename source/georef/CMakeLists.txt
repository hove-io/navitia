SET(GEOREF_SRC
    georef.h
    georef.cpp
    street_network.h
    street_network.cpp
)

add_library(georef ${GEOREF_SRC})

target_link_libraries(georef types)

add_subdirectory(tests)
