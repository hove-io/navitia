#!/bin/bash
cd /navitia/navitia/
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../source
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1))
