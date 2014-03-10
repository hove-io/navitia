find_library(PROJ proj REQUIRED)

SET(SOURCE_LIB
    gtfs_parser.cpp
    fusio_parser.cpp
    osm_tags_reader.cpp
    poi_parser.cpp
    messages_connector.cpp
    fare_parser.cpp
    fare_utils.cpp
    geopal_parser.cpp
    conv_coord.cpp
    external_parser.cpp
    data_cleaner.cpp
)

add_library(connectors ${SOURCE_LIB})
target_link_libraries(connectors ${PROJ})


