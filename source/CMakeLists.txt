project(Kraken)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
set(CMAKE_INSTALL_PREFIX "/")

IF(NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE Debug)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(WIN32)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
    IF(MSVC)
        IF(CMAKE_CL_64)
            SET(LIB_PREFIX "C:/lib64/")
        ELSE(CMAKE_CL_64)
            SET(LIB_PREFIX "C:/lib32/")
        ENDIF(CMAKE_CL_64)
        SET(BOOST_ROOT "${LIB_PREFIX}boost/")

        # Sur un système ayant les 2 versions 32 et 64 bits on considère que Qt est installé à un endroit précis
        FIND_PROGRAM(QMAKE "${LIB_PREFIX}qt-4.7/bin/qmake.exe")
        IF(QMAKE)
            SET(QT_QMAKE_EXECUTABLE "${LIB_PREFIX}qt-4.7/bin/qmake.exe")
        ENDIF(QMAKE)
    ELSEIF(MINGW)
        SET (BOOST_INCLUDEDIR "c:/mingw/include")
    ENDIF(MSVC)
ENDIF(WIN32)



IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "-Wall -pedantic -Wextra -std=c++0x -Woverloaded-virtual -Wundef -rdynamic ")
    SET(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAG} --coverage -g")
    SET(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS} --coverage")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libstdc++  -ferror-limit=10 -pthread")
    SET(CMAKE_C_FLAGS "-ferror-limit=10 -I/usr/local/include/c++/v1 -pthread")
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -fno-omit-frame-pointer")



#gestion des fixtures pour les test unitaire
SET(FIXTURES_DIR ${CMAKE_SOURCE_DIR}/../fixtures/)


INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(iconv.h HAVE_ICONV_H)
FIND_PACKAGE(Boost 1.40.0 REQUIRED)


SET(KRAKEN_VERSION_MAJOR 0)
SET(KRAKEN_VERSION_MINOR 60)
SET(KRAKEN_VERSION_PATCH 0)

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
#add_subdirectory(connectors)
#add_subdirectory(ratimake)
add_subdirectory(pyed)
add_subdirectory(sql)


add_subdirectory(scripts)


#on prend simpleamqpclient en lib static
SET(BUILD_STATIC_LIBS ON CACHE BOOL "Build Rabbitmqc as static library")
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SimpleAmqpClient as static library")
SET(BUILD_API_DOCS OFF CACHE BOOL "don't build dpc of librabbimq-c")
SET(Rabbitmqc_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/librabbitmq-c/librabbitmq/")
add_subdirectory(third_party/librabbitmq-c)
add_subdirectory(third_party/SimpleAmqpClient)

#on récupére le systéme sur lequel on se trouve pour définir les dépendances
execute_process(COMMAND lsb_release -a
    COMMAND grep Codename
    COMMAND sed -e "s/.*:\\s\\([:alpha:]*\\)/\\1/"
    OUTPUT_VARIABLE lsb_release
    OUTPUT_STRIP_TRAILING_WHITESPACE)

#on génére le fichier config.h
configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
SET(CPACK_PACKAGE_VERSION_MAJOR ${KRAKEN_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${KRAKEN_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${KRAKEN_VERSION_PATCH})
SET(CPACK_GENERATOR DEB)

set(CPACK_PACKAGING_INSTALL_PREFIX "/")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Vincent Lara <vincent.lara@canaltp.fr>")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
SET(CPACK_DEBIAN_PACKAGE_NAME "kraken")
SET(CPACK_DEBIAN_PACKAGE_SECTION "unstable")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Kraken ed Jörmungandr")
SET(CPACK_DEBIAN_PACKAGE_SECTION "devel")

IF(${lsb_release} STREQUAL "precise")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libzmq1, libboost-date-time1.48.0, libboost-serialization1.48.0, libboost-regex1.48.0, liblog4cplus-1.0-4, libprotobuf7, libboost-thread1.48.0, libboost-program-options1.48.0, libboost-filesystem1.48.0, python-zmq, python-werkzeug, python-shapely, libqt4-core, libqt4-sql, python-protobuf, libpqxx-3.1")
ELSE()
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libzmq1, libboost-date-time1.49.0, libboost-serialization1.49.0, libboost-regex1.49.0, liblog4cplus-1.0-4, libprotobuf7, libboost-thread1.49.0, libboost-program-options1.49.0, libboost-filesystem1.49.0, python-zmq, python-werkzeug, python-shapely, libqt4-core, libqt4-sql, python-protobuf, libpqxx-3.1")
ENDIF()

INCLUDE(CPack)
