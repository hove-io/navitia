#!/bin/bash
pushd source/navitia-proto
protoc --python_out=../navitiacommon/navitiacommon  type.proto response.proto request.proto task.proto stat.proto
popd
