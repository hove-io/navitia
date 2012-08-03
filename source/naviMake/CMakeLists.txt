FIND_PACKAGE(Boost 1.40.0 COMPONENTS program_options filesystem system REQUIRED)

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


add_subdirectory(tests)
