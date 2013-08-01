#!/bin/bash
#You have to be logged as postgres to execute this script
if [ $# -ne 1 ]
then
    echo 'fichier de conf requis en paramétre'
    exit 1
fi

if [ ! -r $1 ]
then
    echo "$1 n'est pas lisible"
    exit 2
fi

#on s'arrete en cas d'erreur
set -e
source $1
#on rapatrie les variables suivantes:
#   - hostname
#   - username
#   - dbname
#   - PGPASSWORD est exporter


#installation de postgis sur la base

psql --set ON_ERROR_STOP=1 --dbname=$dbname -f "$postgis_dir/postgis.sql"

psql --set ON_ERROR_STOP=1 --dbname=$dbname -f "$postgis_dir/spatial_ref_sys.sql"
