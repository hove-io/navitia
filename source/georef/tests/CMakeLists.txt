FIND_PACKAGE(Boost 1.44 COMPONENTS unit_test_framework REQUIRED)


SET(GEOREF_SRC_TEST
    georef_test.cpp
    builder.h
    builder.cpp
)
add_library(georef_test ${GEOREF_SRC_TEST})
target_link_libraries(georef_test ${Boost_LIBRARIES} georef utils)
