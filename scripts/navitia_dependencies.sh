#!/bin/bash
set -e
apt clean
apt update --fix-missing
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
bash "$(dirname "$0")/install_protoc.sh"
