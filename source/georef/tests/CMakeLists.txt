FIND_PACKAGE(Boost 1.44 COMPONENTS unit_test_framework REQUIRED)

add_executable(georef_test georef_test.cpp)
target_link_libraries(georef_test ${Boost_LIBRARIES} georef)
