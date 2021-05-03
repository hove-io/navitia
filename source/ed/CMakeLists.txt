SET(SOURCE_LIB
        data.cpp
        types.cpp
        build_helper.cpp
)

add_library(ed ${SOURCE_LIB})
target_link_libraries(ed types fare data)

FIND_LIBRARY(OSMPBF osmpbf)

add_library(osm2ed_lib osm2ed.cpp)
target_link_libraries(osm2ed_lib ed transportation_data_import ${OSMPBF} protobuf z ${Boost_PROGRAM_OPTIONS_LIBRARY})

set(ED_LINK_LIBS ${NAVITIA_ALLOCATOR} ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(osm2ed osm2ed_main.cpp)
target_link_libraries(osm2ed osm2ed_lib ${ED_LINK_LIBS})

add_library(transportation_data_import ed_persistor.cpp)
target_link_libraries(transportation_data_import connectors ${PQXX_LIB} utils)

add_executable(gtfs2ed gtfs2ed.cpp)
target_link_libraries(gtfs2ed transportation_data_import ${ED_LINK_LIBS})

add_executable(fusio2ed fusio2ed.cpp)
target_link_libraries(fusio2ed transportation_data_import ${ED_LINK_LIBS})

add_library(fare2ed_lib fare2ed.cpp)
target_link_libraries(fare2ed_lib transportation_data_import)

add_executable(fare2ed fare2ed_main.cpp)
target_link_libraries(fare2ed fare2ed_lib ${ED_LINK_LIBS})

add_library(ed2nav_lib ed2nav.cpp ed_reader.cpp)
target_link_libraries(ed2nav_lib connectors types)

add_executable(ed2nav ed2nav_main.cpp)
target_link_libraries(ed2nav ed2nav_lib ${ED_LINK_LIBS})

add_executable(geopal2ed geopal2ed.cpp)
target_link_libraries(geopal2ed transportation_data_import ${ED_LINK_LIBS})

add_executable(poi2ed poi2ed.cpp)
target_link_libraries(poi2ed transportation_data_import ${ED_LINK_LIBS})

add_executable(synonym2ed synonym2ed.cpp)
target_link_libraries(synonym2ed transportation_data_import ${ED_LINK_LIBS})

set(ED_TARGETS_TO_INSTALL gtfs2ed osm2ed ed2nav fusio2ed fare2ed geopal2ed poi2ed synonym2ed)
install(TARGETS ${ED_TARGETS_TO_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
add_subdirectory(connectors)

add_custom_target(ed_executables DEPENDS ${ED_TARGETS_TO_INSTALL})

# Ed integration tests with docker, build with the docker_test target
# and not with the default target not to have a strong docker dependency
add_subdirectory(docker_tests EXCLUDE_FROM_ALL)
