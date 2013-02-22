#!/bin/sh

#si le fichier de lock existe, un traitement est deja en cours, on arréte
lock_file="$0.lock"
if [ -f $lock_file ]
then
    echo "binarisation deja en cours"
    exit 0
fi

config_file="$0-config.sh"


if [ ! -r $config_file ]
then
    echo "$config_file n'existe  pas ou n'est pas lisible"
    exit 1
fi

#equivalent à "source" mais portable sur sh
. $config_file

if [ ! -d $source_dir ] 
then
    echo "source_dir ($source_dir) n'est pas un répertoire"
    exit 2
fi

if [ ! -d $destination_dir ]
then
    echo "destination_dir ($destination_dir) n'est pas un répertoire"
    exit 2
fi

if [ ! -d $error_dir ]
then
    echo "error_dir ($error_dir) n'est pas un répertoire"
    exit 2
fi

if [ ! -d $data_dir ]
then
    echo "data_dir ($data_dir) n'est pas un répertoire"
    exit 2
fi

if [ ! -x $navimake ]
then
    echo "navimake ($navimake) n'est pas executable"
    exit 2
fi

#création du fichier de lock
touch $lock_file


for file in `find $source_dir -name '*.zip'`
do
    temp_dir=$destination_dir/`date +%Y%m%d-%H%M%S`
    mkdir $temp_dir

    echo "binarisation de $file dans $temp_dir "

    unzip $file -d $temp_dir >> "$temp_dir/bina.log" 2>&1
    if [ $? -ne 0 ]
    then
        mv $file $error_dir
        rm -f $lock_file
        echo "impossible de dézipper $file"
        exit 4
    fi

    if [ -d "$street_network" ]
    then
        navimake_options=" --topo $street_network "
    else 
        if [ -r "$street_network" ] 
        then
            navimake_options=" --osm $street_network "
        else
            echo "street_network ($street_network) non trouvé"
        fi 
    fi
    

    `$navimake -i $temp_dir -o $data_dir/$data_filename $navimake_options >> "$temp_dir/bina.log" 2>&1`
    if [ $? -ne 0 ]
    then
        mv $file $error_dir
        rm -f $lock_file
        echo "erreurs lors de la binarisation"
        exit 3
    fi
    echo "generation du checksum"
    echo `md5sum $data_dir/$data_filename | cut -d' ' -f1` > "$data_dir/$data_filename.md5"
    rm -f $file


done

rm -f $lock_file
