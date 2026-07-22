#!/bin/bash
pushd source/navitia-proto
protoc-python --python_out=../navitiacommon/navitiacommon type.proto response.proto request.proto task.proto stat.proto
2to3 --no-diffs -w ../navitiacommon/navitiacommon/*_pb2.py
popd
