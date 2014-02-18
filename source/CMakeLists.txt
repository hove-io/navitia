project(Kraken)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

SET(KRAKEN_VERSION_MAJOR 0)
SET(KRAKEN_VERSION_MINOR 82)
SET(KRAKEN_VERSION_PATCH 2)

IF(NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE Debug)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "-Wall -pedantic -Wextra -std=c++0x -Woverloaded-virtual -Wundef -rdynamic -g")
    SET(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAG} --coverage -g")
    SET(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS} --coverage")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libstdc++  -ferror-limit=10 -pthread -ftemplate-depth=512")
    SET(CMAKE_C_FLAGS "-ferror-limit=10 -I/usr/local/include/c++/v1 -pthread")
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -fno-omit-frame-pointer")

#Fixture directory for unit testing
SET(FIXTURES_DIR ${CMAKE_SOURCE_DIR}/../fixtures/)


set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
FIND_PACKAGE(Boost 1.47.0 COMPONENTS unit_test_framework thread regex
    serialization date_time filesystem system regex chrono iostreams
    program_options REQUIRED)



#ajout a l'include path le repertoirre de compilation, pour géré le config.h
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_SOURCE_DIR}
                     ${CMAKE_SOURCE_DIR}/../
                     ${Boost_INCLUDE_DIRS}
                     ${CMAKE_SOURCE_DIR}/third_party/SimpleAmqpClient/src/
)
link_directories(${Boost_LIBRARY_DIRS})


ENABLE_TESTING()

add_subdirectory(utils)
add_subdirectory(type)
add_subdirectory(ptreferential)
add_subdirectory(autocomplete)
add_subdirectory(ed)
add_subdirectory(fare)
add_subdirectory(visu)
add_subdirectory(proximity_list)
add_subdirectory(kraken)
add_subdirectory(vptranslator)
add_subdirectory(lz4_filter)
add_subdirectory(routing)
add_subdirectory(georef)
add_subdirectory(validator)
add_subdirectory(time_tables)
add_subdirectory(jormungandr)
add_subdirectory(sql)
add_subdirectory(disruption)

add_subdirectory(scripts)

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckIncludeFileCXX)
set(CMAKE_REQUIRED_INCLUDES "/usr/include")
CHECK_INCLUDE_FILES(iconv.h HAVE_ICONV_H)
CHECK_INCLUDE_FILE_CXX("log4cplus/loggingmacros.h" HAVE_LOGGINGMACROS_H)

configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

## TEST
add_test(connectors nosetests --where=${CMAKE_CURRENT_SOURCE_DIR}/connectors)
set_tests_properties(connectors PROPERTIES ENVIRONMENT
    PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/connectors)

#ADD_TEST(jormungandr_pep8 pep8 --repeat --exclude=*_pb2.py --ignore=E711,E712 ${CMAKE_SOURCE_DIR}/jormungandr/)
#ADD_TEST(tyr_pep8 pep8 --repeat --exclude=*_pb2.py ${CMAKE_SOURCE_DIR}/tyr/tyr)

#ADD_TEST(sindri_pep8 pep8 --repeat --exclude=*_pb2.py --ignore=E711,E712 ${CMAKE_SOURCE_DIR}/sindri/sindri)
#ADD_TEST(tyr_pep8 pep8 --repeat --exclude=*_pb2.py --ignore=E711,E712 ${CMAKE_SOURCE_DIR}/tyr/tyr)
#ADD_TEST(connectors_pep8 pep8 --repeat --exclude=*_pb2.py --ignore=E711,E712 ${CMAKE_SOURCE_DIR}/connectors)

#THIRD PARTY
#on prend simpleamqpclient en lib static
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
