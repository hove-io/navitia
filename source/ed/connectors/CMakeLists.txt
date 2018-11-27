find_library(PROJ proj REQUIRED)

SET(SOURCE_LIB
    gtfs_parser.cpp
    fusio_parser.cpp
    osm_tags_reader.cpp
    poi_parser.cpp
    fare_parser.cpp
    fare_utils.cpp
    geopal_parser.cpp
    conv_coord.cpp
    projection_system_reader.cpp
    data_cleaner.cpp
    poi_parser.cpp
    synonym_parser.cpp
    tz_db_wrapper.cpp
)

add_library(connectors ${SOURCE_LIB})
target_link_libraries(connectors ${PROJ} tcmalloc)


