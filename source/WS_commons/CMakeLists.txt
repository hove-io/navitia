FIND_PACKAGE(Boost 1.40.0 COMPONENTS thread REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories (${Boost_INCLUDE_DIRS})
SET(LIBS ${Boost_LIBRARIES})

IF(FCGI)
  FIND_LIBRARY(FCGI NAMES fcgi REQUIRED)
  LIST(APPEND LIBS FCGI)
  link_directories (${Boost_LIBRARY_DIRS} ${FCGI_LIBRARY_DIRS})
  include_directories (${Boost_INCLUDE_DIRS} ${FCGI_INCLUDE_DIRS})
ENDIF(FCGI)

SET(BENCH_SOURCES fcgi.h isapi.h threadpool.h data_structures.h tests/benchs.cpp)
add_executable (benchs ${BENCH_SOURCES})
target_link_libraries(benchs ${LIBS})

SET(ISAPI_SOURCES fcgi.h isapi.h threadpool.h data_structures.h examples/isapi.cpp)
add_library (isapi MODULE ${ISAPI_SOURCES})
#target_link_libraries(isapi ${LIBS})