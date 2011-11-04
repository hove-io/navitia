FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams program_options REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} 
                      ${CMAKE_SOURCE_DIR}/utils
                      ${CMAKE_SOURCE_DIR}
         #             ${CMAKE_SOURCE_DIR}/fastlz_filter
         #             ${CMAKE_SOURCE_DIR}/../third_party/fastlz
                      ${CMAKE_SOURCE_DIR}/../third_party
)

SET(SOURCE_LIB
        connectors.cpp 
        connectors.h 
        data.h 
        ${CMAKE_SOURCE_DIR}/utils/csv.cpp 
        data.cpp 
        types.cpp 
        gtfs_parser.cpp
        gtfs_parser.h
        #${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c
        ${CMAKE_SOURCE_DIR}/utils/encoding_converter.cpp
        bdtopo_parser.cpp
        bdtopo_parser.h
)

SET(SOURCE_NAVIMAKE
    main.cpp
)

add_library(connectors ${SOURCE_LIB})
target_link_libraries(connectors ${Boost_LIBRARIES} types)

add_executable (navimake ${SOURCE_NAVIMAKE})
target_link_libraries(navimake ${Boost_LIBRARIES} connectors)

add_subdirectory(tests)
