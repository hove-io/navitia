FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework serialization date_time regex thread iostreams REQUIRED)
FIND_PACKAGE(WS_Commons)
find_package(Protobuf REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} 
    ${CMAKE_SOURCE_DIR} 
    ${CMAKE_CURRENT_BINARY_DIR}/../type
)

#MAKE_WS(trie trie.cpp)


MAKE_WS(navitia ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h navitia_ws.cpp LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} types)
