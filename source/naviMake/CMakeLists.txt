FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} 
                        ${CMAKE_SOURCE_DIR}/utils 
                        ${CMAKE_SOURCE_DIR} 
                        ${CMAKE_SOURCE_DIR}/fastlz_filter
)

SET(SOURCE
        main.cpp 
        connectors.cpp 
        connectors.h 
        data.h 
        ${CMAKE_SOURCE_DIR}/utils/csv.cpp 
        data.cpp 
        types.cpp 
        ${CMAKE_SOURCE_DIR}/fastlz_filter/fastlz.c
)
 

add_executable (BO_test ${SOURCE})
target_link_libraries(BO_test ${Boost_LIBRARIES})
