#!/usr/bin/env bash

file=/usr/src/app/jormungandr.wsgi
monitor_cache2="name=monitor,items=100"


function show_help() {
    cat << EOF
Usage: ${0##*/} -m monitor-process -r max-requests
    -m      [0|1] activate monitor-process
    -r      max-requests before reload for jormungandr worker
    -g      Optional: gormungandr url for route_schedules API (Example: http://gormungandr)
    -v      Optional: gormungandr version API : route_schedules(1), route_schedules and journeys(2)
EOF
}

while getopts "m:r:c:g:v:h" opt; do
    case $opt in
        m) monitor_processes=$OPTARG
            ;;
        r) app_max_requests=$OPTARG
            ;;
        c) jormun_cache_items=$OPTARG
            ;;
        g) gormungandr_url=$OPTARG
            ;;
        v) gormungandr_version=$OPTARG
            ;;
        h|\?)
            show_help
            exit 1
            ;;
    esac
done


if [[ -z $app_max_requests ]]
then
  max_requests=""
else
  max_requests=" --max-requests ${app_max_requests} "
fi

if [[ -z $monitor_processes ]]
then
  monitor_processes=0
fi

if [[ -z $jormun_cache_items ]]
then
  jormun_cache_items=2048
fi

jormungandr_cache2="name=jormungandr,items=${jormun_cache_items}"

# The 'cpp' protobuf python implementation does not exist anymore since protobuf 5,
# importing it fails. 'upb' is the native implementation shipped with the wheel.
if [ "$PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION" == "cpp" ]
then
  echo "PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp is not supported anymore, using upb instead"
  export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=upb
fi

if [[ ! -z $gormungandr_url ]] && [[ ! -z $gormungandr_version ]];
then
  echo "export GORMUNGANDR_URL=$gormungandr_url" >> /etc/apache2/envvars
  echo "export GORMUNGANDR_VERSION=$gormungandr_version" >> /etc/apache2/envvars
fi
# run apache2
service apache2 start
if [ $? == 1 ]
then
  echo "Error: failed to start apache2";
  exit 1
fi

# run UWSGI
if [ $monitor_processes -eq 1 ]
then
  echo "!!!!!!!!!!!!!!!!!!!!! Start Jormungandr with monitoring service !!!!!!!!!!!!!!!!!!!!!"
  # JORMUNGANDR_IS_PUBLIC is set to True only for the use of /v1/backends_status
  uwsgi --cache2 $jormungandr_cache2 $max_requests --http :9090 --stats :5050 --lazy-apps --file $file & JORMUNGANDR_IS_PUBLIC=True uwsgi --cache2 $monitor_cache2 --http :9091 --lazy-apps --file $file --processes 1 --listen 5
else
  echo "!!!!!!!!!!!!!!!!!!!!! Start Jormungandr without monitoring service !!!!!!!!!!!!!!!!!!!!!"
  uwsgi  --cache2 $jormungandr_cache2 $max_requests --http :9090 --stats :5050 --lazy-apps --file $file
fi

if [ $? == 1 ]
then
  echo "Error: Jormungandr was not launched";
  exit 1
fi

exec "$@"
