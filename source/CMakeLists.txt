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
	
	# Sur un système ayant les 2 versions 32 et 64 bits on considère que Qt est installé à un endroit précis
	FIND_PROGRAM(QMAKE "${LIB_PREFIX}qt-4.7/bin/qmake.exe")
	IF(QMAKE)
		SET(QT_QMAKE_EXECUTABLE "${LIB_PREFIX}qt-4.7/bin/qmake.exe")
	ENDIF(QMAKE)
  ELSEIF(MINGW)
    SET (BOOST_INCLUDEDIR "c:/mingw/include")
  ENDIF(MSVC)  
ENDIF(WIN32)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "-Wall -pedantic -Wextra -std=c++0x")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(MSVC)
	
ENDIF(MSVC)

ENABLE_TESTING()

add_subdirectory (WS_commons)
add_subdirectory (http_lib)
add_subdirectory (gateway)
add_subdirectory (SqlServer)
add_subdirectory (benchmarks)
add_subdirectory (type)
add_subdirectory (first_letter)
add_subdirectory (log)
add_subdirectory (naviMake)
add_subdirectory (fastlz_filter)
add_subdirectory (fare)
add_subdirectory(gateway_mod)
