FIND_PACKAGE(Boost 1.40.0 COMPONENTS program_options REQUIRED)

SET(SOURCE_LIB
        connectors.cpp 
        data.cpp 
        types.cpp 
        gtfs_parser.cpp
        bdtopo_parser.cpp
        build_helper.cpp
)

SET(SOURCE_NAVIMAKE
    main.cpp
)

add_library(connectors ${SOURCE_LIB})
target_link_libraries(connectors ${Boost_LIBRARIES} types utils)

add_executable (navimake ${SOURCE_NAVIMAKE})
target_link_libraries(navimake ${Boost_LIBRARIES} connectors utils)

add_subdirectory(tests)
