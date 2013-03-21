
SET(SOURCE_LIB 
        data.cpp 
        types.cpp 
        gtfs_parser.cpp
        bdtopo_parser.cpp
        build_helper.cpp
        osm2nav.cpp
        osm_tags_reader.cpp
        adapted.cpp
		poi_parser.cpp
)
add_library(connectors ${SOURCE_LIB})

FIND_PACKAGE(Qt4 REQUIRED QtCore QtSql)
include_directories(${QT_INCLUDE_DIR})

FIND_PACKAGE(Boost 1.48.0 COMPONENTS program_options filesystem system serialization date_time thread regex REQUIRED)
FIND_LIBRARY(OSMPBF osmpbf)
add_executable(navimake main.cpp)
target_link_libraries(navimake connectors connectors_at ${OSMPBF} types data georef pb_lib utils routing ${Boost_LIBRARIES} ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY})

add_subdirectory(tests)

INSTALL_TARGETS(/usr/bin/ navimake)
