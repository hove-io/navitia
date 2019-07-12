#!/bin/bash

docker build -t navitia-builder --build-arg BUILD_TYPE=$BUILD_TYPE $(pwd)/navitia/builder
docker run --rm -v /var/run/docker.sock:/var/run/docker.sock -v $(pwd)/navitia/navitia:/build/navitia navitia-builder "$@"
