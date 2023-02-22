#!/bin/bash
SOURCE_PROTO=${SOURCE_PROTO:-"source/navitia-proto"}
OUTPUT_PROTO=${SOURCE_PROTO:-"../navitiacommon/navitiacommon"}
pushd $SOURCE_PROTO
protoc --python_out=$OUTPUT_PROTO  type.proto response.proto request.proto task.proto stat.proto
2to3 -w $OUTPUT_PROTO/*_pb2.py
popd
