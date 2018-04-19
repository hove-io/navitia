# Usefull to share cmake global var in codes :
# FIXTURES_DIR
# CMAKE_BUILD_TYPE
# GIT_REVISION

configure_file("${CMAKE_SOURCE_DIR}/conf.h.cmake" "${CMAKE_BINARY_DIR}/conf.h")
configure_file("${CMAKE_SOURCE_DIR}/config.cpp.cmake" "${CMAKE_BINARY_DIR}/config.cpp")
add_library(config "${CMAKE_BINARY_DIR}/config.cpp")
