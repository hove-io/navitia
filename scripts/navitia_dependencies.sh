#!/bin/bash
set -e
apt clean
rm -rf /var/lib/apt/lists/*
printf 'deb [check-valid-until=no] http://archive.debian.org/debian/ bullseye main\ndeb [check-valid-until=no] http://archive.debian.org/debian/ bullseye-updates main\ndeb [check-valid-until=no] http://snapshot.debian.org/archive/debian-security/20260831T000000Z bullseye-security main\n' > /etc/apt/sources.list
apt-get -o Acquire::Check-Valid-Until=false update
apt install -y -o Acquire::Retries=10 \
    libpq5 \
    git libgeos-c1v5 ca-certificates curl \
    protobuf-compiler 2to3 \
    libgoogle-perftools-dev libboost-all-dev \
    libprotobuf-dev \
    liblog4cplus-dev libzmq3-dev libpqxx-dev \
    python3-setuptools  postgresql-client  \
    gettext-base jq libssl-dev \
    libosmpbf-dev libproj-dev gcc g++ cmake \
    virtualenv clang-format ccache

# modern protoc (protoc-python) used to generate the python protobuf bindings
curl -fsSL -o /tmp/protoc-python.zip https://github.com/protocolbuffers/protobuf/releases/download/v29.5/protoc-29.5-linux-x86_64.zip
python3 -m zipfile -e /tmp/protoc-python.zip /tmp/protoc-python && install -Dm0755 /tmp/protoc-python/bin/protoc /usr/local/bin/protoc-python
