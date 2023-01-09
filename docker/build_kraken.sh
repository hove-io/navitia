#!/bin/bash
cd /build/navitia/
DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -b
cd ..
chmod -R 777 .
mv *.deb /build/navitia/
