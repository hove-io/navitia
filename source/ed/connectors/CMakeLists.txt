SET(SOURCE_LIB
    gtfs_parser.cpp
    osm2nav.cpp
    osm_tags_reader.cpp
    poi_parser.cpp
    messages_connector.cpp
    extcode2uri.cpp
)

add_library(connectors ${SOURCE_LIB})


