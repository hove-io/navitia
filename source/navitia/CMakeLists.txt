FIND_PACKAGE(WS_Commons)

MAKE_WS(navitia navitia_ws.cpp LIBS utils types routing firstletter proximitylist streetnetwork ptreferential interface time_tables)

INSTALL_TARGETS(/navitia navitia)
