#!/usr/bin/env bash

app_processes=${1-1}
monitor_processes=${2-0}

file=/usr/src/app/jormungandr.wsgi

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
	uwsgi --cache2 "name=jormungandr,items=2048" --http 0.0.0.0:9090 --stats 0.0.0.0:5050 --file $file --processes $app_processes & uwsgi --cache2 "name=monitor,items=100" --http 0.0.0.0:9091 --file $file --processes 1 --listen 5
else
  echo "!!!!!!!!!!!!!!!!!!!!! Start Jormungandr without monitoring service !!!!!!!!!!!!!!!!!!!!!"
	uwsgi  --cache2 "name=jormungandr,items=2048" --http 0.0.0.0:9090 --stats 0.0.0.0:5050 --file $file --processes $app_processes
fi

if [ $? == 1 ]
then
  echo "Error: Jormungandr was not launched";
  exit 1
fi

exec "$@"
