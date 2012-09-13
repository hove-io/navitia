FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
find_package(Protobuf REQUIRED)
find_library(PROJ proj REQUIRED)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)


SET(TYPES_SRC
    type.cpp
    ${PROTO_SRCS}
    pb_converter.cpp
)
add_library(types ${TYPES_SRC})
target_link_libraries(types ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} ${PROJ}  utils)

SET(DATA_SRC
    data.cpp
    ${CMAKE_SOURCE_DIR}/../third_party/lz4/lz4.c
    pt_data.cpp
    locker.cpp
)

add_library(data ${DATA_SRC})
target_link_libraries(data ${Boost_LIBRARIES} types streetnetwork routing)

FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework  REQUIRED)
add_executable(type_test tests/test.cpp)
target_link_libraries(type_test ${Boost_LIBRARIES} proj types)
ADD_TEST(type_test ${EXECUTABLE_OUTPUT_PATH}/type_test --report_level=no)
