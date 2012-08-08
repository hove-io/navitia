FIND_PACKAGE(Boost 1.40.0 COMPONENTS graph unit_test_framework date_time REQUIRED)

SET(FARE_SRC fare.cpp fare.h  ${CMAKE_SOURCE_DIR}/utils/csv.cpp)
IF(NOT MSVC)
	SET(FARE_SRC ${FARE_SRC} ${CMAKE_SOURCE_DIR}/utils/csv.cpp ${CMAKE_SOURCE_DIR}/utils/encoding_converter.cpp)
ENDIF(NOT MSVC)

add_library(fare ${FARE_SRC})

add_executable (fare_tests tests/test.cpp fare.h)
target_link_libraries(fare_tests ${Boost_LIBRARIES} fare)
ADD_TEST(test_fare ${EXECUTABLE_OUTPUT_PATH}/fare_tests --report_level=no)

FIND_PACKAGE(WS_Commons)
MAKE_WS(fare_ws fare_ws.cpp LIBS fare types utils)
