
FIND_PACKAGE(Qt4 REQUIRED QtCore QtSql)
include_directories(${QT_INCLUDE_DIR})

FIND_PACKAGE(Boost 1.48.0 COMPONENTS program_options filesystem system serialization date_time thread REQUIRED)

add_executable(ratimake ratimake.cpp)
target_link_libraries(ratimake connectors_at types data utils ${Boost_LIBRARIES} ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY} log4cplus)

#add_subdirectory(tests)

#INSTALL_TARGETS(/usr/bin/ ratimake)
