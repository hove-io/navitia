FIND_PACKAGE(WS_Commons)
FIND_PACKAGE(RapidXml REQUIRED)
FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization regex thread filesystem REQUIRED)
FIND_PACKAGE(log4cplus REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/http_lib ${RAPIDXML_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
link_directories (${NAViTiA_SOURCE_DIR}/http_dll) 

SET(SOURCE_FILES gateway.cpp gateway.h ../type/type.h ../type/const.h stat.cpp stat.h)
MAKE_WS(gateway ${SOURCE_FILES} LIBS http ${LOG4CPLUS_LIBS})
