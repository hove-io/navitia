#!/bin/bash
cd /build/navitia/
DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -b
cd ..
mv *.deb /build/navitia/
chmod  777 /build/navitia/*.deb
chmod -R 777 /build/navitia/build_packages/
