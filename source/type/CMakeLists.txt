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


SET(TYPES_SRC type.h type.cpp data.h data.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_library(types ${TYPES_SRC})


add_executable (test_gtfs test.cpp gtfs_parser.cpp gtfs_parser.h ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c)
target_link_libraries(test_gtfs ${Boost_LIBRARIES} types)


SET(PTREF_SRC ptreferential.h ptreferential.cpp indexes.h where.h reflexion.h)
add_library(ptreferential ${PTREF_SRC})
target_link_libraries(ptreferential ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} types)

add_executable(ptref ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c ptref_cli.cpp ptref_graph.cpp)
target_link_libraries(ptref ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} pthread readline ptreferential types)



FIND_PACKAGE(WS_Commons)
MAKE_WS(WS_ptref ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c ptref_ws.cpp ptref_graph.cpp LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} ptreferential types)


add_executable(navijkstra navijkstra.cpp ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c)
target_link_libraries(navijkstra ${Boost_LIBRARIES} types)

add_subdirectory(tests)
