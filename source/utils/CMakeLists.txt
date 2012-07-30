FIND_PACKAGE(log4cplus REQUIRED)
include_directories (${LOG4CPLUS_INCLUDE_DIR})
FIND_PACKAGE(Boost 1.40.0 COMPONENTS thread REQUIRED)

add_library(utils csv.cpp encoding_converter.cpp logger.cpp configuration.cpp timer.cpp backtrace.cpp locker.cpp)
target_link_libraries(utils ${LOG4CPLUS_LIBS} ${Boost_LIBRARIES} )
