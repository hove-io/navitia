#!/bin/bash

if [ $# -eq 1 ]
then
    if [ ! -r $1 ]
    then
        echo "cannot read $1"
        exit 2
    fi
    echo 'loading settings from $1'
    source $1
fi

# stop on errors
set -e


function mandatory() {
var=$1
if [ -z "${!var}" ]
then
    echo "Error: $1 not set"
    exit 2
fi
}

mandatory username
mandatory server
mandatory dbname
mandatory PGPASSWORD

if [ "a$sql_dir" == "a" ]
then
    sql_dir=`dirname $0`/../sql/ed
fi

pushd $sql_dir

tempfile=$(mktemp)

#we build a big file with all sql scripts
#and set them all in one transaction
echo "begin transaction;" > $tempfile

cat *.sql >> $tempfile

echo "commit;" >> $tempfile

psql --set ON_ERROR_STOP=1 -h $server $dbname --username=$username -f $tempfile

rm $tempfile

popd
unset PGPASSWORD
