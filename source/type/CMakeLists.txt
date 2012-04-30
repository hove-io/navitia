FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
find_package(Protobuf REQUIRED)


PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)


SET(TYPES_SRC
    type.cpp
    data.cpp
    ${PROTO_SRCS}
    ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c
    pt_data.cpp)
add_library(types ${TYPES_SRC})
target_link_libraries(types ${Boost_LIBRARIES} proj streetnetwork)

