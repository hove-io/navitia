#!/usr/bin/env bash

# run apache2
service apache2 start
if [ $? == 1 ]
then
  echo "Error: failed to start apache2";
  exit 1
fi
# run UWSGI
uwsgi --http 0.0.0.0:9090 --file /usr/src/app/jormungandr.wsgi

if [ $? == 1 ]
then
  echo "Error: Jormungandr was not launched";
  exit 1
fi

exec "$@"
