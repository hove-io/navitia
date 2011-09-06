FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
find_package(Protobuf REQUIRED)
link_directories (${Boost_LIBRARY_DIRS})
include_directories ( ${Boost_INCLUDE_DIRS}
                      ${CMAKE_SOURCE_DIR}/fastlz_filter
                      ${CMAKE_SOURCE_DIR}/../third_party
                      ${CMAKE_SOURCE_DIR}/../third_party/fastlz
                      ${PROTOBUF_INCLUDE_DIRS}
                      ${CMAKE_CURRENT_BINARY_DIR}
)
 

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)


SET(TYPES_SRC type.h type.cpp data.h data.cpp ${PROTO_SRCS} ${PROTO_HDRS} ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c)
add_library(types ${TYPES_SRC})


add_executable(navijkstra navijkstra.cpp )
target_link_libraries(navijkstra ${Boost_LIBRARIES} types)

#add_subdirectory(tests)
