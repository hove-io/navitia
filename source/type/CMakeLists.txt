find_package(Protobuf REQUIRED)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto response.proto request.proto)


add_library(pb_lib ${PROTO_SRCS} pb_converter.cpp)
target_link_libraries(pb_lib ${PROTOBUF_LIBRARY})
add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS --python_out=${CMAKE_SOURCE_DIR}/jormungandr type.proto
    response.proto request.proto
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/type )

add_library(types type.cpp
    message.cpp datetime.cpp
)

SET(DATA_SRC
    data.cpp
    ${CMAKE_SOURCE_DIR}/../third_party/lz4/lz4.c
    pt_data.cpp
)

add_library(data ${DATA_SRC})

FIND_PACKAGE(Boost 1.48.0 COMPONENTS serialization unit_test_framework  date_time filesystem system regex REQUIRED)
add_executable(type_test tests/test.cpp)
target_link_libraries(type_test types data utils ${Boost_LIBRARIES} log4cplus)

ADD_TEST(type_test ${EXECUTABLE_OUTPUT_PATH}/type_test --report_level=no)

add_executable(datetime_test tests/datetime.cpp)
target_link_libraries(datetime_test ${Boost_LIBRARIES} log4cplus)
ADD_TEST(datetime_test ${EXECUTABLE_OUTPUT_PATH}/datetime_test --report_level=no)


add_executable(accessible_test tests/accessible.cpp)
target_link_libraries(accessible_test ${Boost_LIBRARIES} log4cplus)
ADD_TEST(accessible_test ${EXECUTABLE_OUTPUT_PATH}/accessible_test --report_level=no)
