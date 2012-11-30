add_library(georef georef.h
    georef.cpp
    street_network.h
    street_network.cpp
    builder.h
    builder.cpp)
target_link_libraries(georef types)

add_subdirectory(tests)
