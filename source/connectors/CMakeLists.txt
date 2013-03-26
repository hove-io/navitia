SET(SOURCE_LIB
    adjustit_connector.cpp
)

FIND_PACKAGE(Qt4 REQUIRED QtCore QtSql)
include_directories(${QT_INCLUDE_DIR})

add_library(connectors_at ${SOURCE_LIB})


