FIND_PACKAGE(Boost 1.40.0 COMPONENTS program_options filesystem system REQUIRED)
FIND_LIBRARY(OSMPBF osmpbf)

SET(SOURCE_LIB
        connectors.cpp 
        data.cpp 
        types.cpp 
        gtfs_parser.cpp
        bdtopo_parser.cpp
        build_helper.cpp
        firstletterparser.cpp
        osm2nav.cpp
        osm_tags_reader.cpp
)

SET(SOURCE_NAVIMAKE
    main.cpp
)

add_library(connectors ${SOURCE_LIB})
target_link_libraries(connectors ${Boost_LIBRARIES} data)

add_executable (navimake ${SOURCE_NAVIMAKE})
target_link_libraries(navimake ${Boost_LIBRARIES} connectors ${OSMPBF})

add_executable (navimake_fl main_firstletter.cpp)
target_link_libraries(navimake_fl ${Boost_LIBRARIES} connectors)

add_subdirectory(tests)

INSTALL_TARGETS(/navimake navimake)
