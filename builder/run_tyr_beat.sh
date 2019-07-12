#!/bin/bash

# we need to wait for the database to be ready
while ! pg_isready --host=database; do
    echo "waiting for postgres to be ready"
    sleep 1;
done

export TYR_CONFIG_FILE=/srv/navitia/settings.py
export PYTHONPATH=.:../navitiacommon

#db migration
python ./manage_tyr.py db upgrade

exec celery beat -A tyr.tasks
