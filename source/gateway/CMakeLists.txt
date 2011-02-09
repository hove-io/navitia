FIND_PACKAGE(WS_Commons)

FIND_PACKAGE(RapidXml REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/http_lib ${RAPIDXML_INCLUDE_DIR} )
link_directories (${NAViTiA_SOURCE_DIR}/http_dll ) 

SET(SOURCE_FILES gateway.cpp gateway.h)
#MAKE_WS(gateway ${SOURCE_FILES} LIBS http)
MAKE_WS(QT qgateway ${SOURCE_FILES} LIBS http)
