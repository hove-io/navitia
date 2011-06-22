FIND_PACKAGE(Boost 1.40.0 COMPONENTS graph unit_test_framework REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories (${Boost_INCLUDE_DIRS}
                     ${CMAKE_SOURCE_DIR}
                     ${CMAKE_SOURCE_DIR}/utils )
 
add_executable (fare main.cpp fare.cpp fare.h ${CMAKE_SOURCE_DIR}/utils/csv.cpp ${CMAKE_SOURCE_DIR}/utils/encoding_converter.cpp)


add_executable (fare_tests fare.cpp fare.h test.cpp ${CMAKE_SOURCE_DIR}/utils/csv.cpp ${CMAKE_SOURCE_DIR}/utils/encoding_converter.cpp)
target_link_libraries(fare_tests ${Boost_LIBRARIES})
ADD_TEST(test_fare ${EXECUTABLE_OUTPUT_PATH}/fare_tests --report_level=no)

FIND_PACKAGE(WS_Commons)
MAKE_WS(fare_ws fare_ws.cpp fare.cpp fare.h ${CMAKE_SOURCE_DIR}/utils/csv.cpp ${CMAKE_SOURCE_DIR}/utils/encoding_converter.cpp)
