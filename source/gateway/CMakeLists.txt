SET (BOOST_INCLUDEDIR c:/mingw/include)
FIND_PACKAGE(Boost 1.44.0 COMPONENTS system thread REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/http_lib
    ${CMAKE_SOURCE_DIR}/WS_commons 
    ${Boost_INCLUDE_DIRS})
link_directories (${NAViTiA_SOURCE_DIR}/http_dll ${Boost_LIBRARY_DIRS} ) 


IF(WIN32)
	ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
ENDIF(WIN32)

add_executable (gateway gateway.cpp)

IF(UNIX)
        target_link_libraries(gateway boost_system boost_thread http fcgi)
ELSE(UNIX)
        target_link_libraries(gateway http fcgi)
ENDIF(UNIX)
