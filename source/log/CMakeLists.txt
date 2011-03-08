FIND_PACKAGE(log4cplus REQUIRED)

add_executable(logger main.cpp)
target_link_libraries(logger ${LOG4CPLUS_LIBS})
include_directories (${LOG4CPLUS_INCLUDE_DIR})
