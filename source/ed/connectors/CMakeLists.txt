SET(SOURCE_LIB
    adjustit_connector.cpp
    gtfs_parser.cpp
    osm2nav.cpp
    osm_tags_reader.cpp
    poi_parser.cpp
)

FIND_PACKAGE(Qt4 REQUIRED QtCore QtSql)
include_directories(${QT_INCLUDE_DIR})

add_library(connectors ${SOURCE_LIB})


