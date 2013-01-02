FIND_PACKAGE(WS_Commons)
MAKE_WS(WS_Example zmq.h threadpool.h data_structures.h fcgi.h isapi.h examples/example.cpp baseworker.h LIBS utils)
#MAKE_WS(qWS_Example QT threadpool.h data_structures.h qt/main.h qt/mainwindow.h examples/example.cpp baseworker.h LIBS types utils)
