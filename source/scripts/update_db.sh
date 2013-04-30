#!/bin/bash

#en cas d'erreur on s'arrete
set -e

. settings.sh

cd $sql_dir

tempfile=$(mktemp)

#on construit un gros fichier en assemblant tous les scripts sql, et on encapsule dans une transaction
echo "begin transaction;" > $tempfile

cat *.sql >> $tempfile

echo "commit;" >> $tempfile

psql --set ON_ERROR_STOP=1 -h $server $dbname --username=$username -f $tempfile

rm $tempfile


unset PGPASSWORD
