link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

add_library (mssql STATIC mssql.cpp mssql.h)	
target_link_libraries(mssql sybdb)

add_executable (test_mssql tests.cpp)
target_link_libraries(test_mssql mssql)
