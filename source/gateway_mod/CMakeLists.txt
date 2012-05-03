FIND_PACKAGE(WS_Commons)
find_package(Protobuf REQUIRED)
FIND_PACKAGE(log4cplus REQUIRED)

SET(SOURCE_FILES 
    context.h
    gateway.cpp
    pool.cpp
    navitia.cpp
    interface.cpp
)
MAKE_WS(gateway_mod ${SOURCE_FILES} LIBS curlpp ${LOG4CPLUS_LIBS} curl types)
