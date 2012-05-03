SET(PTREF_SRC ptreferential.cpp indexes.h where.h reflexion.h ptref_graph.cpp)
add_library(ptreferential ${PTREF_SRC})
target_link_libraries(ptreferential types)

add_executable(ptref ptref_cli.cpp)
target_link_libraries(ptref ptreferential readline)

FIND_PACKAGE(WS_Commons)
MAKE_WS(WS_ptref ptref_ws.cpp LIBS ptreferential)

