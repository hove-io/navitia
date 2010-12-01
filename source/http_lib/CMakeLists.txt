link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

add_library (http http.cpp)	

IF(WIN32)
	ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
ENDIF(WIN32)

IF(UNIX)
	target_link_libraries(http boost_system)
ENDIF(UNIX)
