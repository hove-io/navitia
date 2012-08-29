FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework  REQUIRED)


add_library(interface renderer.h renderer.cpp pb_utils.cpp pb_utils.h)
target_link_libraries(interface types)
