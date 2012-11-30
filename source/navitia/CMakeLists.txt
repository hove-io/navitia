FIND_PACKAGE(WS_Commons)
FIND_PACKAGE(Boost 1.40.0 COMPONENTS filesystem serialization date_time thread REQUIRED)
MAKE_WS(navitia navitia_ws.cpp LIBS utils firstletter proximitylist streetnetwork ptreferential interface time_tables pb_lib data routing types ${Boost_LIBRARIES})

INSTALL_TARGETS(/navitia navitia)
