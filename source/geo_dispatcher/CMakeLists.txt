FIND_PACKAGE(WS_Commons)

MAKE_WS(geo_dispatcher geo_dispatcher.cpp LIBS geos_c curl types utils)


INSTALL_TARGETS(/geo_dispatcher geo_dispatcher)

