#!/usr/bin/env bash

file=/usr/src/app/jormungandr.wsgi
jormungandr_cache2="name=jormungandr,items=2048"
monitor_cache2="name=monitor,items=100"


function show_help() {
    cat << EOF
Usage: ${0##*/} -m monitor-process -r max-requests
    -m      [0|1] activate monitor-process
    -r      max-requests before reload for jormungandr worker
EOF
}

while getopts "m:r:h" opt; do
    case $opt in
        m) monitor_processes=$OPTARG
            ;;
        r) app_max_requests=$OPTARG
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
  uwsgi --cache2 $jormungandr_cache2 $max_requests --http :9090 --stats :5050 --file $file & uwsgi --cache2 $monitor_cache2 --http :9091 --file $file --processes 1 --listen 5
else
  echo "!!!!!!!!!!!!!!!!!!!!! Start Jormungandr without monitoring service !!!!!!!!!!!!!!!!!!!!!"
  uwsgi  --cache2 $jormungandr_cache2 $max_requests --http :9090 --stats :5050 --file $file
fi

if [ $? == 1 ]
then
  echo "Error: Jormungandr was not launched";
  exit 1
fi

exec "$@"
