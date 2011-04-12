FIND_PACKAGE(WS_Commons)
FIND_PACKAGE(RapidXml REQUIRED)
FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization regex thread filesystem REQUIRED)
FIND_PACKAGE(log4cplus REQUIRED)
#FIND_LIBRARY(pqxx REQUIRED)

include_directories (${RAPIDXML_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})

SET(SOURCE_FILES gateway.cpp gateway.h ../type/type.h ../type/type.cpp ../type/const.h stat.cpp stat.h)
MAKE_WS(gateway ${SOURCE_FILES} LIBS ${LOG4CPLUS_LIBS} curlpp)
