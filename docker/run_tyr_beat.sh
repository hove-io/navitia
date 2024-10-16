#!/bin/bash

ed_db_config() {
  dbname=$1
  user=$2
  password=$3
  db_host=$4
  port=$5

  # database schema migration
  alembic_file=/tmp/${dbname}_ed_migration.ini
  INSTANCE=$dbname USER=$user PASSWORD=$password DATABASE_HOST=$db_host PORT=$port envsubst < /templates/ed_migration.ini > $alembic_file
  alembic -c $alembic_file upgrade head

  echo "ed db: ${dbname} upgraded"
}

upgrade_cities_db() {
  db_uri=$1

  # Prepare the upgrade file for the cities db in the docker-compose
  cd /usr/share/navitia/cities/

  alembic_file=/usr/share/navitia/cities/alembic.ini
  CITIES_DATABASE_URI=$db_uri envsubst < /templates/cities_migration.ini > $alembic_file
  PYTHONPATH=. alembic -c $alembic_file upgrade head

  echo "cities db upgraded"
}

wait_for_db() {
  db_host=$1

  # we need to wait for the database to be ready
  while ! pg_isready --host=${db_host}; do
      echo "waiting for tyr db: ${db_host} to be ready"
      sleep 1;
  done

  echo "${db_host} is online"
}

wait_for_db ${TYR_ED_DATABASE_HOST}
wait_for_db ${TYR_CITIES_DATABASE_HOST}

# db migration for cities db
upgrade_cities_db ${TYR_CITIES_DATABASE_URI}

echo "updating ed db"
# db migration for each instance

while read var ; do
  if [[ $var == 'TYR_INSTANCE_'* ]]; then
    config=`echo $var | cut -d "=" -f 2`
    db_host=$(echo $config | jq -r '.database.host')
    dbname=$(echo $config | jq -r '.database.dbname')
    user=$(echo $config | jq -r '.database.username')
    pwd=$(echo $config | jq -r '.database.password')
    port=$(echo $config | jq -r '.database.port')

    ed_db_config $dbname $user $pwd $db_host $port
  fi
done < <(env)


exec celery -A tyr.tasks beat
