FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework REQUIRED)
add_library(proximitylist proximity_list.h proximitylist_api.cpp)
add_subdirectory(tests)
