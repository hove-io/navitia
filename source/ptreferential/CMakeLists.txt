FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
find_package(Protobuf REQUIRED)
link_directories (${Boost_LIBRARY_DIRS})
include_directories ( ${Boost_INCLUDE_DIRS}
                      ${CMAKE_SOURCE_DIR}/type
                      ${CMAKE_SOURCE_DIR}
                      ${CMAKE_SOURCE_DIR}/../third_party
                      ${CMAKE_CURRENT_BINARY_DIR}/../type
)


SET(PTREF_SRC ptreferential.h ptreferential.cpp indexes.h where.h reflexion.h  ptref_graph.cpp)
add_library(ptreferential ${PTREF_SRC})
target_link_libraries(ptreferential ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} types)

add_executable(ptref  ptref_cli.cpp)
target_link_libraries(ptref ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} pthread readline ptreferential types)



FIND_PACKAGE(WS_Commons)
MAKE_WS(WS_ptref ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h ptref_ws.cpp  LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} ptreferential types)

