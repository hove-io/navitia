FIND_PACKAGE(WS_Commons)

MAKE_WS(navitia ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h navitia_ws.cpp LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} types)
