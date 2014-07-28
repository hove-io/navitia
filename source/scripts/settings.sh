#!/bin/sh

# nom du serveur pgsql
server='localhost'

#nom d'utilisateur pgsql
username='navitia'

#nom de la base de données
dbname='navitia'

# http://www.postgresql.org/docs/9.1/static/libpq-envars.html
export PGPASSWORD='navitia'

#emplacement des fichiers SQL
sql_dir=../sql/ed

#emplacement où sont installé les fichier SQL pour postgis
postgis_dir='/usr/share/postgresql/9.1/contrib/postgis-2.0'
