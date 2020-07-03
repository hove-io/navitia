#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "prometheus-cpp::core" for configuration "Release"
set_property(TARGET prometheus-cpp::core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(prometheus-cpp::core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libprometheus-cpp-core.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS prometheus-cpp::core )
list(APPEND _IMPORT_CHECK_FILES_FOR_prometheus-cpp::core "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libprometheus-cpp-core.a" )

# Import target "prometheus-cpp::pull" for configuration "Release"
set_property(TARGET prometheus-cpp::pull APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(prometheus-cpp::pull PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libprometheus-cpp-pull.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS prometheus-cpp::pull )
list(APPEND _IMPORT_CHECK_FILES_FOR_prometheus-cpp::pull "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libprometheus-cpp-pull.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
