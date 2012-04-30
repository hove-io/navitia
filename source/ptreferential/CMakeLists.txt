find_package(Protobuf REQUIRED)

SET(PTREF_SRC ptreferential.cpp indexes.h where.h reflexion.h ptref_graph.cpp)
add_library(ptreferential ${PTREF_SRC})
target_link_libraries(ptreferential ${PROTOBUF_LIBRARY} types)

add_executable(ptref  ptref_cli.cpp)
target_link_libraries(ptref ${PROTOBUF_LIBRARY} pthread readline ptreferential types)



FIND_PACKAGE(WS_Commons)
MAKE_WS(WS_ptref ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h ptref_ws.cpp  LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} ptreferential types)

