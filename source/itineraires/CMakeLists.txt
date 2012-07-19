FIND_PACKAGE(WS_Commons)
find_package(Protobuf REQUIRED)

SET(ITINERAIRE_SRC itineraires.h itineraires.cpp to_proto.h to_proto.cpp)
add_library(itineraires ${ITINERAIRE_SRC})


MAKE_WS(WS_Itineraires ${CMAKE_SOURCE_DIR}/gateway_mod/interface.h ${CMAKE_SOURCE_DIR}/gateway_mod/interface.cpp ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h itineraires_ws.cpp LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} types network itineraires)

