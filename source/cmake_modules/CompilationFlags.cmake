# Arch
if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_|amd)64$")
    message("-- Arch detected: ${CMAKE_SYSTEM_PROCESSOR}")
else("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_|amd)64$")
    message(WARNING "-- ${CMAKE_SYSTEM_PROCESSOR} arch is no maintained for navitia project")
endif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_|amd)64$")

# Release by default
IF(NOT CMAKE_BUILD_TYPE)
    #https://cmake.org/pipermail/cmake/2009-June/030311.html
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Gcc Release flags
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_GNUCXX_WARN_FLAGS "-Wall -Wextra -Woverloaded-virtual -Wundef -pedantic")
    set(CMAKE_GNUCXX_COMMON_FLAGS "-std=c++0x -rdynamic -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_GNUCXX_COMMON_FLAGS} ${CMAKE_GNUCXX_WARN_FLAGS}")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAG} --coverage -g")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif(CMAKE_COMPILER_IS_GNUCXX)

# Clang Release flags
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # it could be great to remove these warnings, but there is so much :
    # -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-conversion
    set(CMAKE_CLANG_WARN_FLAGS "-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors -Wno-documentation -Wno-shadow -Wno-covered-switch-default -Wno-switch-enum -Wno-missing-noreturn -Wno-disabled-macro-expansion -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-conversion -Wno-double-promotion")
    set(CMAKE_CLANG_COMMON_FLAGS "-std=c++11 -stdlib=libstdc++  -ferror-limit=10 -pthread -ftemplate-depth=1024")
    set(CMAKE_CXX_FLAGS "${CMAKE_CLANG_COMMON_FLAGS} ${CMAKE_CLANG_WARN_FLAGS}")
    set(CMAKE_C_FLAGS "-ferror-limit=10 -I/usr/local/include/c++/v1 -pthread")
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -fno-omit-frame-pointer")
