SET(SOURCE_LIB
    gtfs_parser.cpp
    fusio_parser.cpp
    osm_tags_reader.cpp
    poi_parser.cpp
    messages_connector.cpp
    extcode2uri.cpp
    fare_parser.cpp
    fare_utils.cpp
	geopal_parser.cpp
)

add_library(connectors ${SOURCE_LIB})


