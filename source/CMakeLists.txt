project(NAViTiACpp)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

IF(WIN32)
  IF(MSVC)
    SET(BOOST_ROOT "C:/Program Files/boost/boost_1_44")
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
  ELSEIF(MINGW)
    SET(BOOST_ROOT "C:/mingw/boost/")
  ENDIF(MSVC)  
ENDIF(WIN32)


add_subdirectory (WS_commons)
add_subdirectory (http_lib)
add_subdirectory (gateway)
add_subdirectory (SqlServer)

