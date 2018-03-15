project(Kraken)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
set(LOG4CPLUS_FIND_REQUIRED true)
find_package(log4cplus)
find_package(GoogleTcmalloc)
find_package(Protoc)

# add custom target to build docker tests not to have a strong docker dependendy
# obviously to run this target you should have docker installed (and the right to run it)
add_custom_target(docker_test)

if(Boost_VERSION)
    # be silent if already found
    set(Boost_FIND_QUIETLY TRUE)
endif()

#get git revision as the version number
include(GetGitRevisionDescription)
git_describe(GIT_REVISION)

IF(NOT CMAKE_BUILD_TYPE)
    #https://cmake.org/pipermail/cmake/2009-June/030311.html
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "-Wall -pedantic -Wextra -std=c++0x -Woverloaded-virtual -Wundef -rdynamic -g")
    SET(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAG} --coverage -g")
    SET(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS} --coverage")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libstdc++  -ferror-limit=10 -pthread -ftemplate-depth=1024 -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors -Wno-documentation -Wno-shadow -Wno-covered-switch-default -Wno-switch-enum -Wno-missing-noreturn -Wno-disabled-macro-expansion")
    # it could be great to remove these warnings, but there is so much!
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-conversion")
    SET(CMAKE_C_FLAGS "-ferror-limit=10 -I/usr/local/include/c++/v1 -pthread")
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -fno-omit-frame-pointer")

#Fixture directory for unit testing
SET(FIXTURES_DIR "${CMAKE_SOURCE_DIR}/../fixtures/")


set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
FIND_PACKAGE(Boost 1.55.0 COMPONENTS unit_test_framework thread regex
    serialization date_time filesystem system regex chrono iostreams
    program_options REQUIRED)

#boost 1.53/1.54 bugs with local datetime...
#see http://stackoverflow.com/questions/15234527/boost-1-53-local-date-time-compiler-error-with-std-c0x
add_definitions(-DBOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
#decomment for enabling streetnetwork debuging with quantu
#add_definitions(-D_DEBUG_DIJKSTRA_QUANTUM_)

# PQXX
FIND_PATH(PQXX_INCLUDE_DIR pqxx/pqxx)
FIND_LIBRARY(PQXX_LIB pqxx)

#add current compilation dir to include path to handle config.h
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_SOURCE_DIR}"
                     "${CMAKE_SOURCE_DIR}/../"
                     "${Boost_INCLUDE_DIRS}"
                     "${CMAKE_SOURCE_DIR}/third_party/SimpleAmqpClient/src/"
                     "${CMAKE_SOURCE_DIR}/third_party/prometheus-cpp/include/"
                     "${CMAKE_BINARY_DIR}/third_party/prometheus-cpp/lib/cpp"
                     "${PQXX_INCLUDE_DIR}"
)
link_directories(${Boost_LIBRARY_DIRS})

SET(TEST_DEBUG_OUTPUT OFF CACHE BOOL "Shoot test output to stdin")

MACRO(BUILD_BOOST_TEST_ARGS EXE_NAME)
    IF("${Boost_VERSION}" LESS 106200)
        IF(TEST_DEBUG_OUTPUT)
            SET(BOOST_TEST_ARGS --log_level=all)
        ELSE()
            SET(BOOST_TEST_ARGS --log_format=XML --log_sink=results_${EXE_NAME}.xml --log_level=all --report_level=no)
        ENDIF()
    ELSE()
        IF(TEST_DEBUG_OUTPUT)
            SET(BOOST_TEST_ARGS --logger=HRF,all )
        ELSE()
            SET(BOOST_TEST_ARGS --logger=XML,all,results_${EXE_NAME}.xml --report_level=no)
        ENDIF()
    ENDIF()
ENDMACRO(BUILD_BOOST_TEST_ARGS)

MACRO(ADD_BOOST_TEST EXE_NAME)
    BUILD_BOOST_TEST_ARGS(${EXE_NAME})
    ADD_TEST(
        NAME ${EXE_NAME} 
        COMMAND "${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}" ${BOOST_TEST_ARGS} "${ARGN}"
    )
ENDMACRO(ADD_BOOST_TEST)

ENABLE_TESTING()

add_subdirectory(utils)
add_subdirectory(type)
add_subdirectory(ptreferential)
add_subdirectory(autocomplete)
add_subdirectory(ed)
add_subdirectory(fare)
add_subdirectory(proximity_list)
add_subdirectory(kraken)
add_subdirectory(vptranslator)
add_subdirectory(lz4_filter)
add_subdirectory(routing)
add_subdirectory(georef)
add_subdirectory(time_tables)
add_subdirectory(jormungandr)
add_subdirectory(tyr)
add_subdirectory(sql)
add_subdirectory(disruption)
add_subdirectory(calendar)

add_subdirectory(tests)
add_subdirectory(scripts)
add_subdirectory(cities)
add_library(linenoise linenoise/linenoise.c)

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckIncludeFileCXX)
set(CMAKE_REQUIRED_INCLUDES "/usr/include")
CHECK_INCLUDE_FILES(iconv.h HAVE_ICONV_H)
CHECK_INCLUDE_FILE_CXX("log4cplus/loggingmacros.h" HAVE_LOGGINGMACROS_H)

configure_file("${CMAKE_SOURCE_DIR}/conf.h.cmake" "${CMAKE_BINARY_DIR}/conf.h")
configure_file("${CMAKE_SOURCE_DIR}/config.cpp.cmake" "${CMAKE_BINARY_DIR}/config.cpp")
add_library(config "${CMAKE_BINARY_DIR}/config.cpp")

#THIRD PARTY
#build simpleamqpclient as static lib
SET(BUILD_STATIC_LIBS ON CACHE BOOL "Build Rabbitmqc as static library")
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SimpleAmqpClient as static library")
SET(BUILD_API_DOCS OFF CACHE BOOL "don't build doc of librabbimq-c")
SET(BUILD_EXAMPLES OFF CACHE BOOL "don't build example of librabbimq-c")
SET(BUILD_TOOLS OFF CACHE BOOL "don't build tool of librabbimq-c")
SET(BUILD_TESTS OFF CACHE BOOL "don't build tests of librabbimq-c")
SET(CMAKE_INSTALL_PREFIX "")
SET(Rabbitmqc_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/librabbitmq-c/librabbitmq/")
add_subdirectory(third_party/librabbitmq-c)
add_subdirectory(third_party/SimpleAmqpClient)
#prometheus-cpp cmake will refuse to build if the CMAKE_INSTALL_PREFIX is empty
#setting it before will have side effects on how we build packages
SET(CMAKE_INSTALL_PREFIX "/")
add_subdirectory(third_party/prometheus-cpp)
