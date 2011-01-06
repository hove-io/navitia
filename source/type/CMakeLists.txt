FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

add_executable (test_gtfs test.cpp gtfs_parser.cpp gtfs_parser.h type.h type.cpp data.h data.cpp indexes.h)
target_link_libraries(test_gtfs ${Boost_LIBRARIES})
