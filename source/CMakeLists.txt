project(NAViTiACpp)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

IF(WIN32)
  SET(BOOST_ROOT "C:/Program Files/boost/boost_1_44")
ENDIF(WIN32)

add_subdirectory (WS_commons)
add_subdirectory (http_lib)
add_subdirectory (gateway)

