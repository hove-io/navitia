#!/bin/bash
cd /navitia/navitia/
mkdir -p docker_build && cd docker_build
cmake -DCMAKE_BUILD_TYPE=Release ../source
(cd jormungandr && make -j$(($(grep -c '^processor' /proc/cpuinfo)+1)))
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1)) protobuf_files
