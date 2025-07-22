#!/bin/bash

tyr_config() {
  instance_name=$1
  INSTANCE=$instance_name envsubst < templates/tyr_instance.ini > /etc/tyr.d/$instance_name.ini

  mkdir -p /srv/ed/$instance_name
  mkdir -p /srv/ed/output
  mkdir -p /srv/ed/input/$instance_name
}

db_config() {
  instance_name=$1

  # wait for db ready
  while ! pg_isready --host=database; do
    echo "waiting for postgres to be ready"
    sleep 1;
  done

  # database creation
  PGPASSWORD=navitia createdb --host database -U navitia $instance_name
  PGPASSWORD=navitia psql -c 'CREATE EXTENSION postgis;' --host database $instance_name navitia

  # database schema migration
  alembic_file=/srv/ed/$instance_name/ed_migration.ini
  INSTANCE=$instance_name USER=navitia PASSWORD=navitia DATABASE_HOST=database PORT=5432 envsubst < templates/ed_migration.ini > $alembic_file
  alembic -c $alembic_file upgrade head
}

add_instance() {
  instance_name=$1
  echo "adding instance $instance_name"

  # tyr configuration
  tyr_config $instance_name

  # db creation and migration
  db_config $instance_name
}

echo_instance() {
  instance_name=$1
  echo "found instance $instance_name"
}


upgrade_cities_db() {
  # Prepare the upgrade file for the cities db in the docker-compose
  cd /usr/share/navitia/cities/
  sed -i 's/dev/alembic/g' alembic.ini
  sed -i 's/localhost/cities_database/g' alembic.ini

  # Wait for cities db ready
  while ! pg_isready --host=cities_database; do
    echo "waiting for cities_database to be ready"
    sleep 1;
  done

  # Perform the upgrade
  PYTHONPATH=. alembic -c alembic.ini upgrade head

  echo "cities db upgraded"
}

# to add an instance add an environment variable called INSTANCE_${NAME_OF_THE_INSTANCE}
instances=$(env | grep "INSTANCE_"  | sed 's/INSTANCE_\(.*\)=.*/\1/')


for i in $instances; do
  add_instance $i
done

for i in $instances; do
  echo_instance $i
done


echo "all instances configured"

upgrade_cities_db
