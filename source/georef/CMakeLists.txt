SET(GEOREF_SRC georef.cpp 
            georef.h
)

add_library(georef ${GEOREF_SRC})
add_subdirectory(tests)
