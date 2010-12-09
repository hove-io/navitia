FIND_PACKAGE(Boost 1.40.0 COMPONENTS REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories (${Boost_INCLUDE_DIRS})

FIND_PACKAGE(freeTDS REQUIRED)
link_directories (${FreeTDS_LIBRARY_DIR})
include_directories (${FreeTDS_INCLUDE_DIR})

add_library (mssql STATIC mssql.cpp mssql.h)	
target_link_libraries(mssql)

add_executable (test_mssql tests.cpp)
target_link_libraries(test_mssql mssql ${FreeTDS_LIBS})
MESSAGE(STATUS "Using : ${FreeTDS_LIBS}\n and : ${FreeTDS_INCLUDE_DIR}")
