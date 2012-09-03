#!/bin/sh

print_help(){

    echo "$0 <data_source> <destination> <instance_url>"

    echo "data_source: uri vers les données binarisé"
    echo "destination: endroit ou les données doivent etre déposé"
    echo "instance_url: instance ou effectuer un load"

}

echo $#
if [ $# -ne 3 ] 
then
    print_help
    exit 1;
fi

data_source=$1
destination=$2
instance_url=$3

wget -O $destination $data_source

curl "$instance_url/load"

