
SET(SOURCE_LIB
    adjustit_connector.cpp
)

FIND_PACKAGE(Qt4 REQUIRED QtCore QtSql)
include_directories(${QT_INCLUDE_DIR})

add_library(ratimake_lib ${SOURCE_LIB})


FIND_PACKAGE(Boost 1.40.0 COMPONENTS program_options filesystem system serialization date_time thread REQUIRED)

add_executable(ratimake ratimake.cpp)
target_link_libraries(ratimake ratimake_lib types data utils ${Boost_LIBRARIES} ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY})

#add_subdirectory(tests)

#INSTALL_TARGETS(/usr/bin/ ratimake)
