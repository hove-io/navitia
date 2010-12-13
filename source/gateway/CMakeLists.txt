FIND_PACKAGE(WS_Commons)

include_directories (${CMAKE_SOURCE_DIR}/http_lib)
link_directories (${NAViTiA_SOURCE_DIR}/http_dll) 

SET(SOURCE_FILES gateway.cpp gateway.h)
MAKE_WS(gateway ${SOURCE_FILES} LIBS http)