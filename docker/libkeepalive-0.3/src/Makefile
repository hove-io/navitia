#   _ _ _     _                         _ _
#  | (_) |__ | | _____  ___ _ __   __ _| (_)_   _____
#  | | | '_ \| |/ / _ \/ _ \ '_ \ / _` | | \ \ / / _ \
#  | | | |_) |   <  __/  __/ |_) | (_| | | |\ V /  __/
#  |_|_|_.__/|_|\_\___|\___| .__/ \__,_|_|_| \_/ \___|
#                          |_|
#
#  (C) Fabio Busatto <fabio.busatto@gmail.com>

CC=gcc
CFLAGS=-fPIC -ansi -pedantic -Wall
LDFLAGS=-shared -Wl,-soname,libkeepalive.so
LDLIBS=-ldl

default: libkeepalive.so

%.so: %.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f *.so *.o
