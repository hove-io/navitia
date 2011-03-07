FIND_PACKAGE(WS_Commons)
FIND_PACKAGE(freeTDS REQUIRED)
FIND_PACKAGE(RapidXml REQUIRED)
FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization regex thread filesystem REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/http_lib ${RAPIDXML_INCLUDE_DIR} ${FreeTDS_INCLUDE_DIR})
link_directories (${NAViTiA_SOURCE_DIR}/http_dll ${FreeTDS_LIBRARY_DIR}) 

SET(SOURCE_FILES gateway.cpp gateway.h ../type/type.h ../type/const.h stat.cpp stat.h)
MAKE_WS(gateway ${SOURCE_FILES} LIBS http ${FreeTDS_LIBS})
