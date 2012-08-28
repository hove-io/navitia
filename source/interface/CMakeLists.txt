FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework  REQUIRED)


add_library(interface renderer.h renderer.cpp)
target_link_libraries(interface types)
