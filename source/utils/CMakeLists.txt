FIND_PACKAGE(log4cplus REQUIRED)
include_directories (${LOG4CPLUS_INCLUDE_DIR})

add_library(utils csv.cpp encoding_converter.cpp logger.cpp configuration.cpp timer.cpp functions.cpp)
target_link_libraries(utils ${LOG4CPLUS_LIBS})
