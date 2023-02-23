#!/bin/bash
cd /navitia/navitia/
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../source
echo "-----"
echo $(($(grep -c '^processor' /proc/cpuinfo)+1))
echo "-----"
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1)) kraken
