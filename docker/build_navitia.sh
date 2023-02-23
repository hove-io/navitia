#!/bin/bash
echo "*********************"
ls -lt
echo "*********************"
echo `pwd`
echo "*********************"
cd /build/navitia/
navitia_build_dir="$navitia_dir"/build_release
mkdir -p "$navitia_build_dir" && cd "$navitia_build_dir"
echo "*********************"
ls -lt
echo "*********************"
echo `pwd`
echo "*********************"
cmake -DCMAKE_BUILD_TYPE=Release ../source
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1))
