FIND_PACKAGE(WS_Commons)
find_package(Protobuf REQUIRED)

SET(SOURCE_FILES 
    context.h
    gateway.cpp
    pool.cpp
    navitia.cpp
    interface.cpp
)
MAKE_WS(gateway_mod ${SOURCE_FILES} LIBS curlpp utils curl types)
