FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/type ${CMAKE_SOURCE_DIR}/utils)
 

add_executable (BO_test main.cpp connectors.cpp connectors.h data.h ${CMAKE_SOURCE_DIR}/utils/csv.cpp)
target_link_libraries(BO_test ${Boost_LIBRARIES})
