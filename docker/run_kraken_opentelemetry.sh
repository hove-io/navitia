#!/bin/bash

# Adapted from https://docs.docker.com/config/containers/multi-service_container/

# Start opentelemetry exporter
/usr/bin/otelcol-contrib --config=/etc/otelcol/config.yaml &

# Start monitor_kraken
python /monitor_kraken &

# Start kraken
LD_PRELOAD=/libkeepalive/libkeepalive.so /usr/bin/kraken

# Wait for any process to exit
wait -n

# Exit with status of process that exited first
exit $?
