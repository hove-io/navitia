#!/bin/bash

# we need to wait for the database to be ready
while ! pg_isready --host=${TYR_DATABASE_HOST}; do
    echo "waiting for postgres to be ready"
    sleep 1;
done

python /usr/bin/manage_tyr.py db upgrade
echo "MIGRATION IS FINISHED"

uwsgi --mount /=tyr:app --http 0.0.0.0:80
