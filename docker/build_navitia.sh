#!/bin/bash
echo "*********************"
ls /navitia/
echo "*********************"
ls -lt  /navitia/navitia/
echo "*********************"
cd /navitia/navitia/
navitia_build_dir="$navitia_dir"/build_release
mkdir -p "$navitia_build_dir" && cd "$navitia_build_dir"
cmake -DCMAKE_BUILD_TYPE=Release ../source
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1))
