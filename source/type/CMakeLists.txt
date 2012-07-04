FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
find_package(Protobuf REQUIRED)


PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)


SET(TYPES_SRC
    type.cpp
    data.cpp
    ${PROTO_SRCS}
    ${CMAKE_SOURCE_DIR}/../third_party/lz4/lz4.c
    pt_data.cpp
    to_proto.cpp
    pb_utils.cpp
)
add_library(types ${TYPES_SRC})
target_link_libraries(types ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} proj streetnetwork utils)

