# Arch
if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_|amd)64$")
    message("-- Arch detected: ${CMAKE_SYSTEM_PROCESSOR}")
else("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_|amd)64$")
    message(WARNING "-- ${CMAKE_SYSTEM_PROCESSOR} arch is no maintained for navitia project")
endif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_|amd)64$")

# Release by default
if(NOT CMAKE_BUILD_TYPE)
    #https://cmake.org/pipermail/cmake/2009-June/030311.html
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message("-- By default, Build type is Release")
else(NOT CMAKE_BUILD_TYPE)
    message("-- Build type : ${CMAKE_BUILD_TYPE}")
endif(NOT CMAKE_BUILD_TYPE)

# compile tests flag
if(NOT SKIP_TESTS)
    message("-- Tests compilation actived")
else(NOT SKIP_TESTS)
    message("-- Tests compilation deactived")
endif(NOT SKIP_TESTS)

# Gcc Release flags
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_GNUCXX_WARN_FLAGS "-Wall -Wextra -Woverloaded-virtual -Wundef -pedantic")
    set(CMAKE_GNUCXX_COMMON_FLAGS "-std=c++14 -rdynamic -g -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_GNUCXX_COMMON_FLAGS} ${CMAKE_GNUCXX_WARN_FLAGS}")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAG} --coverage -fprofile-arcs -ftest-coverage  -g")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif(CMAKE_COMPILER_IS_GNUCXX)


option(USE_LD_GOLD "Use GNU gold linker" ON)

if(USE_LD_GOLD AND "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version OUTPUT_VARIABLE stdout ERROR_QUIET)
  if("${stdout}" MATCHES "GNU gold")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fuse-ld=gold")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
  else()
    message(WARNING "GNU gold linker isn't available, using the default system linker.")
  endif()
endif()

option(CCACHE "Use ccache if available" ON)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE AND CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Clang Release flags
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # it could be great to remove these warnings, but there is so much :
    # -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-conversion
    set(CMAKE_CLANG_WARN_FLAGS "-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors -Wno-documentation -Wno-shadow -Wno-covered-switch-default -Wno-switch-enum -Wno-missing-noreturn -Wno-disabled-macro-expansion -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-conversion -Wno-missing-braces")
    set(CMAKE_CLANG_COMMON_FLAGS "-std=c++14 -stdlib=libstdc++  -ferror-limit=10 -pthread -ftemplate-depth=1024")
    set(CMAKE_CXX_FLAGS "${CMAKE_CLANG_COMMON_FLAGS} ${CMAKE_CLANG_WARN_FLAGS}")
    set(CMAKE_C_FLAGS "-ferror-limit=10 -I/usr/local/include/c++/v1 -pthread")
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1")

# Strip symbols
if(STRIP_SYMBOLS)
    message("-- Strip symbols actived")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif(STRIP_SYMBOLS)

set(NAVITIA_ALLOCATOR "tcmalloc")

option(USE_SANITIZER "build with the desired sanitizer enabled. usual values: address, thread, leak, undefined" OFF)
if(USE_SANITIZER)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=${USE_SANITIZER} -fno-sanitize=vptr")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${USE_SANITIZER} -fno-sanitize=vptr")
    add_definitions("-DNO_FORCE_MEMORY_RELEASE")
    set(NAVITIA_ALLOCATOR "")
endif(USE_SANITIZER)

find_file(PROJ_FILE_NAME NAMES proj_api.h)
if(NOT PROJ_FILE_NAME)
    message(FATAL_ERROR "-- proj_api.h not found")
endif()
file(STRINGS ${PROJ_FILE_NAME} _PROJ_LIB_VERSION REGEX "^#define PJ_VERSION ([0-9]+)([0-9]+)([0-9]+)")
STRING (REGEX REPLACE "^#define PJ_VERSION " "" _PROJ_LIB_VERSION "${_PROJ_LIB_VERSION}")
message("-- Found Proj Library : ${_PROJ_LIB_VERSION}")
IF("${_PROJ_LIB_VERSION}" LESS 600)
    add_definitions("-DPROJ_API_VERSION_MAJOR_4")
ELSEIF("${_PROJ_LIB_VERSION}" GREATER_EQUAL 600)
    add_definitions("-DPROJ_API_VERSION_MAJOR_6")
ENDIF()
