# Install script for directory: /home/runner/work/navitia/navitia/source

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/runner/work/navitia/navitia/utils/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/type/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/ptreferential/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/autocomplete/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/ed/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/fare/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/proximity_list/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/kraken/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/vptranslator/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/lz4_filter/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/routing/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/georef/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/time_tables/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/jormungandr/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/tyr/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/sql/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/disruption/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/calendar/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/scripts/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/cities/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/tools/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/equipment/cmake_install.cmake")
  include("/home/runner/work/navitia/navitia/tests/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/runner/work/navitia/navitia/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
