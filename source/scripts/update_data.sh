#!/bin/sh

print_help(){

    echo "$0 <data_source> <destination> <init_file>"

    echo "data_source: uri vers les données binarisé"
    echo "destination: endroit ou les données doivent etre déposé"
    echo "init_file : script sur lequel il faut effectuer un reload"

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
init_file=$3

if [ -r "$data_source.md5" ]
then
    md5_remote=`cat "$data_source.md5"`
else
    md5_remote=`curl "$data_source.md5"`
fi
md5_local=`md5 $destination`

echo $md5_remote
echo $md5_local

if [ $md5_local != $md5_remote ]
then

    if [ -r $data_source ]
    then
        cp $data_source $destination
    else
        wget -O $destination $data_source
    fi
    sudo $init_file reload

fi

exit 0
