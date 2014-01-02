SET(DISRUPTION_SRC
    disruption_api.cpp
    disruption.cpp
)
add_library(disruption_api ${DISRUPTION_SRC})

add_subdirectory(tests)
