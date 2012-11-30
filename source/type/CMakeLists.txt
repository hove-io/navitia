FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)

find_package(Protobuf REQUIRED)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)

add_library(pb_lib ${PROTO_SRCS} pb_converter.cpp)
target_link_libraries(pb_lib ${PROTOBUF_LIBRARY})

add_library(types type.cpp)
target_link_libraries(types ${Boost_LIBRARIES})

SET(DATA_SRC
    data.cpp
    ${CMAKE_SOURCE_DIR}/../third_party/lz4/lz4.c
    pt_data.cpp
    locker.cpp
)

add_library(data ${DATA_SRC})
target_link_libraries(data ${Boost_LIBRARIES} types)

FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework  REQUIRED)
add_executable(type_test tests/test.cpp)
target_link_libraries(type_test ${Boost_LIBRARIES}  types utils)
ADD_TEST(type_test ${EXECUTABLE_OUTPUT_PATH}/type_test --report_level=no)
