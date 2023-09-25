#!/usr/bin/env bash

app_processes=${1-1}
monitor_processes=${2-0}

file=/usr/src/app/jormungandr.wsgi
jormungandr_ini=/usr/src/app/jormungandr.ini
monitor_ini=/usr/src/app/monitor.ini

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
	uwsgi --ini $jormungandr_ini --file $file --processes $app_processes & uwsgi --http --ini $monitor_ini --file $file --processes $monitor_processes
else
  echo "!!!!!!!!!!!!!!!!!!!!! Start Jormungandr without monitoring service !!!!!!!!!!!!!!!!!!!!!"
	uwsgi --ini $jormungandr_ini --file $file --processes $app_processes
fi

if [ $? == 1 ]
then
  echo "Error: Jormungandr was not launched";
  exit 1
fi

exec "$@"
