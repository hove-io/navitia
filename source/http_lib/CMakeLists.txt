FIND_PACKAGE(Boost 1.40.0 COMPONENTS system REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

add_library (http http.cpp)	
target_link_libraries(http ${Boost_LIBRARIES})
