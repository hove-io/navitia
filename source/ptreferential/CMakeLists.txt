SET(PTREF_SRC ptreferential.cpp where.h reflexion.h ptref_graph.cpp)
add_library(ptreferential ${PTREF_SRC})

add_subdirectory(tests)
