find_package(Protobuf REQUIRED)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto response.proto request.proto task.proto realtime.proto)


add_library(pb_lib ${PROTO_SRCS} pb_converter.cpp)
target_link_libraries(pb_lib ${PROTOBUF_LIBRARY})
add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS --python_out=${CMAKE_SOURCE_DIR}/navitiacommon/navitiacommon type.proto
    response.proto request.proto task.proto realtime.proto
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/type )

add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS --python_out=${CMAKE_SOURCE_DIR}/sindri/sindri type.proto
    task.proto realtime.proto
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/type )

add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS --python_out=${CMAKE_SOURCE_DIR}/connectors/connectors type.proto
    task.proto realtime.proto
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/type )

add_library(types type.cpp message.cpp datetime.cpp)

SET(DATA_SRC
    data.cpp
    ${CMAKE_SOURCE_DIR}/third_party/lz4/lz4.c
    pt_data.cpp
)

add_library(data ${DATA_SRC})

SET(BOOST_LIBS ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}  ${Boost_REGEX_LIBRARY})

add_executable(type_test tests/test.cpp)
target_link_libraries(type_test types data utils ${BOOST_LIBS} log4cplus)

ADD_TEST(type_test ${EXECUTABLE_OUTPUT_PATH}/type_test --report_level=no)

add_executable(datetime_test tests/datetime.cpp)
target_link_libraries(datetime_test ${BOOST_LIBS} log4cplus)
ADD_TEST(datetime_test ${EXECUTABLE_OUTPUT_PATH}/datetime_test --report_level=no)


add_executable(accessible_test tests/accessible.cpp)
target_link_libraries(accessible_test ${BOOST_LIBS} log4cplus)
ADD_TEST(accessible_test ${EXECUTABLE_OUTPUT_PATH}/accessible_test --report_level=no)
