FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
find_package(Protobuf REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS}
                      ${CMAKE_SOURCE_DIR}/fastlz_filter
                      ${CMAKE_SOURCE_DIR}/third_party
                      ${PROTOBUF_INCLUDE_DIRS}
                      ${CMAKE_CURRENT_BINARY_DIR}
)
 
add_executable (test_gtfs test.cpp gtfs_parser.cpp gtfs_parser.h type.h type.cpp data.h data.cpp ${CMAKE_SOURCE_DIR}/fastlz_filter/fastlz.c)
target_link_libraries(test_gtfs ${Boost_LIBRARIES})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)
add_executable (ptref ptreferential.h ptreferential.cpp type.h type.cpp data.h data.cpp indexes.h where.h reflexion.h ${CMAKE_SOURCE_DIR}/fastlz_filter/fastlz.c ${PROTO_SRCS} ${PROTO_HDRS} ptref_cli.cpp ptref_graph.cpp)
target_link_libraries(ptref ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} readline)



FIND_PACKAGE(WS_Commons)
MAKE_WS(WS_ptref ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h ptreferential.h ptreferential.cpp type.h type.cpp data.h data.cpp indexes.h where.h reflexion.h ${CMAKE_SOURCE_DIR}/fastlz_filter/fastlz.c ${PROTO_SRCS} ${PROTO_HDRS} ptref_ws.cpp ptref_graph.cpp LIBS ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY})


add_executable (navijkstra navijkstra.cpp data.cpp type.cpp ${CMAKE_SOURCE_DIR}/fastlz_filter/fastlz.c )
target_link_libraries(navijkstra ${Boost_LIBRARIES})

add_subdirectory(tests)
