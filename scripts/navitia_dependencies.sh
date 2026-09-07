#!/bin/bash
set -e
apt clean
rm -rf /var/lib/apt/lists/*
apt update
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
