FIND_PACKAGE(Boost 1.40.0 COMPONENTS graph REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS}
                      ${CMAKE_SOURCE_DIR}
)

add_executable (test_sn test.cpp types.h types.cpp ../utils/csv.cpp ../utils/encoding_converter.cpp)
target_link_libraries(test_sn ${Boost_LIBRARIES})
