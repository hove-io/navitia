project(NAViTiACpp)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

IF(WIN32)
  ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
  IF(MSVC)
	IF(CMAKE_CL_64)
		SET(LIB_PREFIX "C:/lib64/")
	ELSE(CMAKE_CL_64)
		SET(LIB_PREFIX "C:/lib32/")
	ENDIF(CMAKE_CL_64)
	
	SET(BOOST_ROOT "${LIB_PREFIX}boost/")
  ELSEIF(MINGW)
    SET (BOOST_INCLUDEDIR "c:/mingw/include")
  ENDIF(MSVC)  
ENDIF(WIN32)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "-Wall -pedantic -Wextra -std=c++0x")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

ENABLE_TESTING()

add_subdirectory (WS_commons)
add_subdirectory (http_lib)
add_subdirectory (gateway)
add_subdirectory (SqlServer)
add_subdirectory (benchmarks)
add_subdirectory (type)
