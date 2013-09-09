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
#en cas d'erreur on s'arrete
set -e

source $1
cd $sql_dir

tempfile=$(mktemp)

#on construit un gros fichier en assemblant tous les scripts sql
# et on encapsule dans une transaction
echo "begin transaction;" > $tempfile

cat *.sql >> $tempfile

echo "commit;" >> $tempfile

psql --set ON_ERROR_STOP=1 -h $server $dbname --username=$username -f $tempfile

rm $tempfile


unset PGPASSWORD
