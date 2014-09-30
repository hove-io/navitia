SET(GEOREF_SRC
    georef.h
    georef.cpp
    street_network.h
    street_network.cpp
    adminref.h
    adminref.cpp
)

add_library(georef ${GEOREF_SRC})

target_link_libraries(georef types proximitylist utils)

add_subdirectory(tests)
