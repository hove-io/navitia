FIND_PACKAGE(Boost 1.40.0 COMPONENTS program_options filesystem system REQUIRED)
#find_package(Protobuf REQUIRED)

#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS osmformat.proto fileformat.proto)

SET(SOURCE_LIB
        connectors.cpp 
        data.cpp 
        types.cpp 
        gtfs_parser.cpp
        bdtopo_parser.cpp
        build_helper.cpp
        firstletterparser.cpp
)

SET(SOURCE_NAVIMAKE
    main.cpp
)

add_library(connectors ${SOURCE_LIB})
target_link_libraries(connectors ${Boost_LIBRARIES} data)

add_executable (navimake ${SOURCE_NAVIMAKE})
target_link_libraries(navimake ${Boost_LIBRARIES} connectors)

add_executable (navimake_fl main_firstletter.cpp)
target_link_libraries(navimake_fl ${Boost_LIBRARIES} connectors)

add_executable (osm2nav osm2nav.cpp)
target_link_libraries(osm2nav data geos_c osmpbf)

add_subdirectory(tests)
