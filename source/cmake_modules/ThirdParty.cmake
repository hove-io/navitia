# Third party

# Third party warnings have to be silent


#
# SimpleAmqpClient
# librabbimq-c
#
include_directories("${CMAKE_SOURCE_DIR}/third_party/SimpleAmqpClient/src/")
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
# prometheus-cpp
#
# We want to deactivate prometheus lib for debian < 8.0 and ubuntu < 16.00
if  ( ((CMAKE_OS_NAME MATCHES "Debian") AND
      (CMAKE_OS_VERSION VERSION_GREATER "8.0"))
    OR
     ((CMAKE_OS_NAME MATCHES "Ubuntu") AND
     (CMAKE_OS_VERSION VERSION_GREATER "16.00"))
    )

    include_directories("${CMAKE_SOURCE_DIR}/third_party/prometheus-cpp/include/"
                        "${CMAKE_BINARY_DIR}/third_party/prometheus-cpp/lib/cpp")
    #prometheus-cpp cmake will refuse to build if the CMAKE_INSTALL_PREFIX is empty
    #setting it before will have side effects on how we build packages
    set(CMAKE_INSTALL_PREFIX "/")
    add_subdirectory(third_party/prometheus-cpp)

    message("-- Add prometheus in third party")

    set(PROMETHEUS_IS_ACTIVED ON)
    # Flag activation for code define
    add_definitions(-DPROMETHEUS_IS_ACTIVED)

else  ( ((CMAKE_OS_NAME MATCHES "Debian") AND
        (CMAKE_OS_VERSION VERSION_GREATER "8.0"))
      OR
        ((CMAKE_OS_NAME MATCHES "Ubuntu") AND
        (CMAKE_OS_VERSION VERSION_GREATER "16.00"))
      )
    message("-- Skip prometheus in third party")
    message(DEPRECATION " ${CMAKE_OS_NAME} ${CMAKE_OS_VERSION} is no longer maintained for navitia project")

    set(PROMETHEUS_IS_ACTIVED OFF)
    # Flag activation for code define (deactivate)
    add_definitions(-DPROMETHEUS_IS_ACTIVED=0)

endif ( ((CMAKE_OS_NAME MATCHES "Debian") AND
        (CMAKE_OS_VERSION VERSION_GREATER "8.0"))
      OR
        ((CMAKE_OS_NAME MATCHES "Ubuntu") AND
        (CMAKE_OS_VERSION VERSION_GREATER "16.00"))
      )
