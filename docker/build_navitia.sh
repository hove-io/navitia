#!/bin/bash
cd /navitia/navitia/
mkdir -p build && cd build
echo "*********************"
ls .
echo "*********************"
ls -lt  ../
echo "*********************"
cmake -DCMAKE_BUILD_TYPE=Release ../source
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1))
