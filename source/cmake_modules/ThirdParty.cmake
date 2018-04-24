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
set(CMAKE_INSTALL_PREFIX "")
set(Rabbitmqc_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/librabbitmq-c/librabbitmq/")
add_subdirectory(third_party/librabbitmq-c)
add_subdirectory(third_party/SimpleAmqpClient)

#
# Include third_party code in system mode, to avoid warnings
#
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/")

#
# prometheus-cpp
#
# We want to deactivate prometheus lib if CMake version < 2.8.12.2
if("${CMAKE_VERSION}" VERSION_GREATER 2.8.12.1)

    include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/prometheus-cpp/include/"
                        "${CMAKE_BINARY_DIR}/third_party/prometheus-cpp/lib/cpp")
    #prometheus-cpp cmake will refuse to build if the CMAKE_INSTALL_PREFIX is empty
    #setting it before will have side effects on how we build packages
    set(CMAKE_INSTALL_PREFIX "/")
    add_subdirectory(third_party/prometheus-cpp)

    message("-- Add prometheus in third party")

    set(ENABLE_PROMETHEUS ON)
    # Flag activation for code define
    add_definitions(-DENABLE_PROMETHEUS=1)

else()
    message("-- CMake version < 2.8.12.2, Skip prometheus in third party")
    message(DEPRECATION " ${CMAKE_OS_NAME} ${CMAKE_OS_VERSION} is no longer maintained for navitia project")

    set(ENABLE_PROMETHEUS OFF)
    # Flag activation for code define (=0)
    add_definitions(-DENABLE_PROMETHEUS=0)

endif()

# Reactivate warnings flags
set(CMAKE_CXX_FLAGS ${TMP_FLAG})
