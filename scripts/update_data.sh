#!/bin/sh

print_help(){

    echo "$0 <data_source> <destination> <instance_url>"

    echo "data_source: uri vers les données binarisé"
    echo "destination: endroit ou les données doivent etre déposé"
    echo "instance_url: instance ou effectuer un load"

}

md5(){
    if [ -r $1 ]
    then
        echo `md5sum $1 | cut -d' ' -f1`
    else
        echo "0"
    fi
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

md5_remote=`curl "$data_source.md5"`
md5_local=`md5 $destination`

echo $md5_remote
echo $md5_local

if [ $md5_local != $md5_remote ]
then

    wget -O $destination $data_source
    curl "$instance_url/load"

fi

