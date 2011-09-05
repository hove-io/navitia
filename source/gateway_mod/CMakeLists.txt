FIND_PACKAGE(WS_Commons)
FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization regex thread filesystem REQUIRED)
find_package(Protobuf REQUIRED)
FIND_PACKAGE(log4cplus REQUIRED)

include_directories (${CMAKE_SOURCE_DIR} ${PROTOBUF_INCLUDE_DIRS} ${LOG4CPLUS_INCLUDE_DIR}) 


PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)

SET(SOURCE_FILES 
    context.h
    gateway.h
    gateway.cpp
    pool.h
    pool.cpp
    navitia.h
    navitia.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    interface.h
    interface.cpp
)
MAKE_WS(gateway_mod ${SOURCE_FILES} LIBS curlpp ${PROTOBUF_LIBRARY} ${LOG4CPLUS_LIBS})
