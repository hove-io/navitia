#!/bin/bash

cd /navitia/navitia/
mkdir -p docker_build && cd docker_build
cmake -DCMAKE_BUILD_TYPE=Release ../source


build_targets() {
    local targets=("$@")
    for file in "${targets[@]}"
    do
        echo "******************** Building $file ********************"
        make -j$(($(grep -c '^processor' /proc/cpuinfo)+1)) $file
        if [ $? -ne 0 ]; then
            echo "Error building $file"
            exit 1
        fi
    done
}

# kraken
build_targets protobuf_files kraken

# mocks for integration tests
build_targets departure_board_test main_routing_test

# some binaries for tyr-worker
build_targets cities ed2nav fare2ed fusio2ed geopal2ed gtfs2ed osm2ed poi2ed



# Build libkeepalive https://libkeepalive.sourceforge.net/
# libkeepalive is a library to keep tcp connection alive. The reason of doing so is that aws LoadBalancer, to which
# kraken is connected, check the aliveness continuously at regular interval of non-customisable 350s
# (https://docs.aws.amazon.com/elasticloadbalancing/latest/network/network-load-balancers.html). If nothing happens during
# this interval, the connection will be dropped. It takes Kraken, by default, 7200s(/proc/sys/net/ipv4/tcp_keepalive_time)
# to be aware of the disconnection. By using libkeepalive, we can override tcp_keepalive_time,
# which is not customisable via docker-container.
(mkdir -p build_libkeepalive && cp -rf ../docker/libkeepalive-0.3/. build_libkeepalive/ && cd build_libkeepalive/ && make )
