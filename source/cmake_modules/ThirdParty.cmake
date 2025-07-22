# Third party

# Third party warnings have to be silent, we skip warnings
set(TMP_FLAG ${CMAKE_CXX_FLAGS})
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_GNUCXX_COMMON_FLAGS} -w")
endif(CMAKE_COMPILER_IS_GNUCXX)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CLANG_COMMON_FLAGS} -Wno-everything")
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

#
# SimpleAmqpClient
# librabbimq-c
#
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/SimpleAmqpClient/src/")
set(BUILD_STATIC_LIBS ON CACHE BOOL "Build Rabbitmqc as static library")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SimpleAmqpClient as static library")
set(BUILD_API_DOCS OFF CACHE BOOL "don't build doc of librabbimq-c")
set(BUILD_EXAMPLES OFF CACHE BOOL "don't build example of librabbimq-c")
set(BUILD_TOOLS OFF CACHE BOOL "don't build tool of librabbimq-c")
set(BUILD_TESTS OFF CACHE BOOL "don't build tests of librabbimq-c")
set(Rabbitmqc_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/librabbitmq-c/librabbitmq/")
set(Rabbitmqc_LIBRARY rabbitmq-static)
add_subdirectory(third_party/librabbitmq-c EXCLUDE_FROM_ALL)
add_subdirectory(third_party/SimpleAmqpClient EXCLUDE_FROM_ALL)

#
# Include third_party code in system mode, to avoid warnings
#
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/")

#
# prometheus-cpp
#
include_directories(SYSTEM "${CMAKE_BINARY_DIR}/third_party/prometheus-cpp/core/include/")
include_directories(SYSTEM "${CMAKE_BINARY_DIR}/third_party/prometheus-cpp/pull/include/")
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/prometheus-cpp/core/include/")
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/prometheus-cpp/pull/include/")
link_directories("${CMAKE_BINARY_DIR}/third_party/prometheus-cpp/lib")
#prometheus-cpp cmake will refuse to build if the CMAKE_INSTALL_PREFIX is empty
#setting it before will have side effects on how we build packages
set(ENABLE_PUSH OFF CACHE INTERNAL "" FORCE)
add_subdirectory(third_party/prometheus-cpp)


# Reactivate warnings flags
set(CMAKE_CXX_FLAGS ${TMP_FLAG})

#
# flann
#
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/flann/src/cpp")

#
# lz4
#
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/lz4")
