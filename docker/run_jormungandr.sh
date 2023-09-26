#!/usr/bin/env bash

file=/usr/src/app/jormungandr.wsgi
jormungandr_cache2="name=jormungandr,items=2048"
monito_cache2="name=monitor,items=100"


function show_help() {
    cat << EOF
Usage: ${0##*/} -j jormungandr-processes -m monitor-processes -r max-requests
    -j      jormungandr-processes
    -m      [0|1] monitor-processes
    -r      max-requests for jormungandr
EOF
}

while getopts "j:m:r:h" opt; do
    case $opt in
        j) app_processes=$OPTARG
            ;;
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
  uwsgi --cache2 $jormungandr_cache2 $max_requests --http :9090 --stats :5050 --file $file --processes $app_processes & uwsgi --cache2 $monito_cache2 --http :9091 --file $file --processes 1 --listen 5
else
  echo "!!!!!!!!!!!!!!!!!!!!! Start Jormungandr without monitoring service !!!!!!!!!!!!!!!!!!!!!"
  uwsgi  --cache2 $jormungandr_cache2 $max_requests --http :9090 --stats :5050 --file $file --processes $app_processes
fi

if [ $? == 1 ]
then
  echo "Error: Jormungandr was not launched";
  exit 1
fi

exec "$@"
