FIND_PACKAGE(WS_Commons)
MAKE_WS(WS_Example threadpool.h data_structures.h fcgi.h isapi.h examples/example.cpp baseworker.h LIBS types)
MAKE_WS(qWS_Example QT threadpool.h data_structures.h fcgi.h isapi.h examples/example.cpp baseworker.h LIBS types)
