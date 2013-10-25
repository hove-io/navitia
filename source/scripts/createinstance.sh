#!/bin/bash


print_help(){

    echo "$0 <config_file> <instance_name> <is_free>"

    echo "config_file: fichier de configuration pour l'accés en base"
    echo "instance_name: nom de l'instance à ajouter"
    echo "is_free: si l'instance est en accés libre (1 ou 0)"

}

if [ $# -ne 3 ]
then
    print_help
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
instance=$2
if [ $3 -eq 1 ]
then
    isfree=true
else
    isfree=false
fi
psql --set ON_ERROR_STOP=1 -h $server $dbname --username=$username -c "INSERT INTO jormungandr.instance (id, name, is_free) VALUES (DEFAULT, '$instance', $isfree);"


unset PGPASSWORD
