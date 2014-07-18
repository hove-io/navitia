if(GOOGLETCMALLOC_INCLUDE_DIR)
    # be silent if already found
    set(GOOGLETCMALLOC_FIND_QUIETLY TRUE)
endif()

find_path(GOOGLETCMALLOC_INCLUDE_DIR google/malloc_extension.h PATH "${LIB_PREFIX}")

if(GOOGLETCMALLOC_INCLUDE_DIR)
    set(GOOGLETCMALLOC_FOUND TRUE)
endif()

if(GOOGLETCMALLOC_FOUND)
    if(NOT GOOGLETCMALLOC_FIND_QUIETLY)
        message(STATUS "Found tcmalloc in ${GOOGLETCMALLOC_INCLUDE_DIR}/google/malloc_extension.h")
    endif()
else()
    message(FATAL_ERROR "Could not find google/malloc_extension.h")
endif()
