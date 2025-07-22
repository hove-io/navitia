#   _ _ _     _                         _ _
#  | (_) |__ | | _____  ___ _ __   __ _| (_)_   _____
#  | | | '_ \| |/ / _ \/ _ \ '_ \ / _` | | \ \ / / _ \
#  | | | |_) |   <  __/  __/ |_) | (_| | | |\ V /  __/
#  |_|_|_.__/|_|\_\___|\___| .__/ \__,_|_|_| \_/ \___|
#                          |_|
#
#  (C) Fabio Busatto <fabio.busatto@gmail.com>

default:
	make -C src/
	make -C test/
	cp src/libkeepalive.so libkeepalive.so
	strip -s libkeepalive.so

clean:
	make -C src/ clean
	make -C test/ clean
	rm -f libkeepalive.so
