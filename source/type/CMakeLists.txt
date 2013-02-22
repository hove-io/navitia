
find_package(Protobuf REQUIRED)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)


add_library(pb_lib ${PROTO_SRCS} pb_converter.cpp)
target_link_libraries(pb_lib ${PROTOBUF_LIBRARY})
add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS --python_out=../frontend type.proto
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/type )

add_library(types type.cpp 
    message.cpp
)

SET(DATA_SRC
    data.cpp
    ${CMAKE_SOURCE_DIR}/../third_party/lz4/lz4.c
    pt_data.cpp
)

add_library(data ${DATA_SRC})

FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework  REQUIRED)
add_executable(type_test tests/test.cpp)
target_link_libraries(type_test ${Boost_LIBRARIES}  types utils)

ADD_TEST(type_test ${EXECUTABLE_OUTPUT_PATH}/type_test --report_level=no)
