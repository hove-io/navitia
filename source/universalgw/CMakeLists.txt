FIND_PACKAGE(WS_Commons)

MAKE_WS(vivendi vivendi.cpp LIBS geos_c utils curl)


INSTALL_TARGETS(/vivendi vivendi)

