if(${CMAKE_VERSION} VERSION_GREATER "3.1.0")
    cmake_policy(SET CMP0054 NEW)
endif()

# Returns a simple string describing the current architecture. Possible
# return values currently include: amd64, x86_64, x86.
# Returns a lowercased version of a given lsb_release field.
MACRO (_LSB_RELEASE field retval)
  EXECUTE_PROCESS (COMMAND lsb_release "--${field}"
  OUTPUT_VARIABLE _output ERROR_VARIABLE _output RESULT_VARIABLE _result)
  IF (_result)
    MESSAGE (FATAL_ERROR "Cannot determine Linux revision! Output from "
    "lsb_release --${field}: ${_output}")
  ENDIF (_result)
  STRING (REGEX REPLACE "^[^:]*:" "" _output "${_output}")
  STRING (TOLOWER "${_output}" _output)
  STRING (STRIP "${_output}" ${retval})
ENDMACRO (_LSB_RELEASE)

# Returns a lowercased version of a given /etc/os-release field.
MACRO (_OS_RELEASE field retval)
  FILE (STRINGS /etc/os-release vars)
  SET (${_value} "${field}-NOTFOUND")
  FOREACH (var ${vars})
    IF (var MATCHES "^${field}=(.*)")
      SET (_value "${CMAKE_MATCH_1}")
      # Value may be quoted in single- or double-quotes; strip them
      IF (_value MATCHES "^['\"](.*)['\"]\$")
        SET (_value "${CMAKE_MATCH_1}")
      ENDIF ()
      BREAK ()
    ENDIF ()
  ENDFOREACH ()

  SET (${retval} "${_value}")
ENDMACRO (_OS_RELEASE)

# Returns a simple string describing the current platform. Possible
# return values currently include: windows_msvc2017; windows_msvc2015;
# windows_msvc2013; windows_msvc2012; macosx; or any value from
# _DETERMINE_LINUX_DISTRO.

# Returns a simple string describing the current Linux distribution
# compatibility. Possible return values currently include:
# ubuntu14.04, ubuntu12.04, ubuntu10.04, centos5, centos6, debian7, debian8.
MACRO (DETERMINE_LINUX_DISTRO)
  IF (EXISTS "/etc/os-release")
      _OS_RELEASE (ID OS_ID)
      _OS_RELEASE (VERSION_ID OS_VERSION)
  ENDIF ()
  IF (NOT ( OS_ID AND OS_VERSION ) )
    FIND_PROGRAM(LSB_RELEASE lsb_release)
    IF (LSB_RELEASE)
      _LSB_RELEASE (id OS_ID)
      _LSB_RELEASE (release OS_VERSION)
    ELSE (LSB_RELEASE)
      MESSAGE (WARNING "Can't determine Linux platform without /etc/os-release or lsb_release")
      SET (OS_ID "unknown")
      SET (OS_VERSION "")
    ENDIF (LSB_RELEASE)
  ENDIF ()
  IF (OS_ID STREQUAL "linuxmint")
    # Linux Mint is an Ubuntu derivative; estimate nearest Ubuntu equivalent
    SET (OS_ID "ubuntu")
    IF (OS_VERSION VERSION_LESS 13)
      SET (OS_VERSION 10.04)
    ELSEIF (OS_VERSION VERSION_LESS 17)
      SET (OS_VERSION 12.04)
    ELSEIF (OS_VERSION VERSION_LESS 18)
      SET (OS_VERSION 14.04)
    ELSE ()
      SET (OS_VERSION 16.04)
    ENDIF ()
  ELSEIF (OS_ID STREQUAL "debian" OR OS_ID STREQUAL "centos" OR OS_ID STREQUAL "rhel")
    # Just use the major version from the CentOS/Debian/RHEL identifier;
    # we don't need different builds for different minor versions.
    STRING (REGEX MATCH "[0-9]+" OS_VERSION "${OS_VERSION}")
  ELSEIF (OS_ID STREQUAL "fedora" AND OS_VERSION VERSION_LESS 26)
    SET (OS_ID "centos")
    SET (OS_VERSION "7")
  ELSEIF (OS_ID MATCHES "opensuse.*" OR OS_ID MATCHES "suse.*" OR OS_ID MATCHES "sles.*")
    SET(OS_ID "suse")
    # Just use the major version from the SuSE identifier - we don't
    # need different builds for different minor versions.
    STRING (REGEX MATCH "[0-9]+" OS_VERSION "${OS_VERSION}")
  ENDIF (OS_ID STREQUAL "linuxmint")
  SET ("${OS_ID}${OS_VERSION}")
ENDMACRO (DETERMINE_LINUX_DISTRO)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message( FATAL_ERROR "-- OS ${CMAKE_SYSTEM_NAME} not supported")
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")

DETERMINE_LINUX_DISTRO()
message("-- OS ${CMAKE_SYSTEM_NAME} ${OS_ID}-${OS_VERSION}")

if(("${OS_ID}" STREQUAL "debian" AND "${OS_VERSION}" LESS 10) OR ("${OS_ID}" STREQUAL "ubuntu" AND "${OS_VERSION}" LESS 20))
    message("-- need for backward compatibility for libpqxx")
    set(PQXX_COMPATIBILITY ON)
    if(PQXX_COMPATIBILITY)
        add_definitions(-DPQXX_COMPATIBILITY)
    endif()
else(("${OS_ID}" STREQUAL "debian" AND "${OS_VERSION}" LESS 10) OR ("${OS_ID}" STREQUAL "ubuntu" AND "${OS_VERSION}" LESS 20))
    message("-- no need for backward compatibility for libpqxx")
endif(("${OS_ID}" STREQUAL "debian" AND "${OS_VERSION}" LESS 10) OR ("${OS_ID}" STREQUAL "ubuntu" AND "${OS_VERSION}" LESS 20))
