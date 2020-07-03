# Install script for directory: /home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/runner/work/navitia/navitia/third_party/SimpleAmqpClient/libSimpleAmqpClient.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/SimpleAmqpClient" TYPE FILE FILES
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/AmqpException.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/AmqpLibraryException.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/AmqpResponseLibraryException.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/BadUriException.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/BasicMessage.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/Channel.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/ConnectionClosedException.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/ConsumerCancelledException.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/ConsumerTagNotFoundException.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/Envelope.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/MessageReturnedException.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/SimpleAmqpClient.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/Table.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/Util.h"
    "/home/runner/work/navitia/navitia/source/third_party/SimpleAmqpClient/src/SimpleAmqpClient/Version.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/lib/pkgconfig/libSimpleAmqpClient.pc")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/lib/pkgconfig" TYPE FILE FILES "/home/runner/work/navitia/navitia/third_party/SimpleAmqpClient/libSimpleAmqpClient.pc")
endif()

