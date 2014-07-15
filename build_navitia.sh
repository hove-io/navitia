#!/bin/sh


# **************************************
# Running Navitia in the blink of an eye
# **************************************
#
# You just need to blink slowly
#
# Here is a step by step guide for running navitia on a Ubuntu 14.04
#
# It's more an install guide but it can help as an out-of-a-box build script
# the prerequisite the run that script is to have cloned the sources repository
#
# git clone https://github.com/CanalTP/navitia
#
# and to be in the cloned repository:
# cd navitia

#stop on errors
set -e

kraken_db_user_password=
navitia_dir=`dirname $(readlink -f $0)`
gtfs_data_dir=
osm_file=

install_dependencies=1

usage()
{
cat << EOF
usage: $0 options

This script setup a running navitia

only the password is mandatory:
 - if no dataset are given a default Paris one will be used
 - by default all dependencies are installed

OPTIONS:
   -h                  Show this message
   -p                  kraken database password
   -g                  gtfs directory 
   -o                  osm directory
   -n                  don't install dependencies
EOF
}

while getopts “hp:g:o:n” OPTION
do
     case $OPTION in
         h)
             usage
             exit 1
             ;;
         p)
             kraken_db_user_password=$OPTARG
             ;;
         g)
             gtfs_data_dir=$OPTARG
             ;;
         o)
             osm_file=$OPTARG
             ;;
         n)
             install_dependencies=
             ;;
         ?)
             usage
             exit
             ;;
     esac
done

if [ -z "$kraken_db_user_password" ]
then
    echo "no database password given, abort"
    exit 1
fi

if [ -z "$gtfs_data_dir" ] || [ -z "$osm_file" ]
then
    echo "no gtfs or osm file given, we'll take a default data set, Paris"

    echo "getting gtfs paris data from data.navitia.io"
    wget -P /tmp http://data.navitia.io/gtfs_paris_20140502.zip
    unzip -d /tmp/gtfs /tmp/gtfs_paris_20140502.zip
    gtfs_data_dir=/tmp/gtfs

    echo "getting paris osm data from metro.teczno.com"
    wget -P /tmp http://osm-extracted-metros.s3.amazonaws.com/paris.osm.pbf
    osm_file=/tmp/paris.osm.pbf
fi

run_dir=$navitia_dir/run
mkdir -p $run_dir

#Hack
#for convenience reason, some submodule links are in ssh (easier to push)
#it is however thus mandatory for the user to have a github access
#for this script we thus change the ssh links to https
sed -i 's,git\@github.com:\([^/]*\)/\(.*\).git,https://github.com/\1/\2,' .gitmodules

#we need to get the submodules
git submodule update --init

#========
#Building
#========
#
#First you need to install all dependecies. 
#
#first the system and the c++ dependencies: 
if [ $install_dependencies ]
then
    sudo apt-get install -y git g++ cmake liblog4cplus-dev libzmq-dev libosmpbf-dev libboost-all-dev libpqxx3-dev libgoogle-perftools-dev libprotobuf-dev python-pip libproj-dev protobuf-compiler libgeos-c1 
    
    sudo apt-get install -y postgresql-9.3 postgresql-9.3-postgis-2.1 #Note: postgres 9.1 and postgis 2.0 would be enough, be postgis 2.1 is easier ton setup 
    
    # then you need to install all python dependencies: ::
    
    sudo pip install -r $navitia_dir/source/jormungandr/requirements.txt
    sudo pip install -r $navitia_dir/source/tyr/requirements.txt
fi

#the build procedure is explained is the install documentation
navitia_build_dir=$navitia_dir/release
mkdir -p $navitia_build_dir && cd $navitia_build_dir
cmake -DCMAKE_BUILD_TYPE=Release ../source
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1))


#=======================
#Setting up the database
#=======================
#
#Each kraken is backed by a postgres database
#
#You need to set up the database
kraken_db_name='navitia'
db_owner='navitia'

kraken_db_name='navitia'
# for the default build we give ownership of the base to a 'navitia' user, but you can do whatever you want here
sudo su postgres -c "psql postgres -tAc \"SELECT 1 FROM pg_roles WHERE rolname='$db_owner'\""  # we check if there is already a user 
if [ ! $? ]
then
sudo su postgres -c "psql -c \"create user $db_owner;alter user $db_owner password '$kraken_db_user_password';\""
else
echo "user $db_owner already exists"
fi

sudo su postgres -c "psql -l" | grep "^ $kraken_db_name "
if [ ! $? ]
then
sudo su postgres -c "psql -c \"create database $kraken_db_name owner $db_owner; \""
sudo su postgres -c "psql -c \"create extension postgis; \" $kraken_db_name"
else 
echo "db $kraken_db_name already exists"
fi

# Then you need to update it's scheme
# For that you can use a configuration file or just add environment variables
# There is an configuration file example in source/script/settings.sh
username=$db_owner server=localhost dbname=$kraken_db_name PGPASSWORD=$kraken_db_user_password $navitia_dir/source/scripts/update_db.sh


#Running
#=======

# ** filling up the database **

# we need to import the gtfs data
#$navitia_build_dir/ed/gtfs2ed -i $gtfs_data_dir --connection-string="host=localhost user=$db_owner password=$kraken_db_user_password"

# we need to import the osm data
#$navitia_build_dir/ed/osm2ed -i $osm_file --connection-string="host=localhost user=$db_owner password=$kraken_db_user_password"

# then we export the database into kraken's custom file format
$navitia_build_dir/ed/ed2nav -o $run_dir/data.nav.lz4 --connection-string="host=localhost user=$db_owner password=$kraken_db_user_password"

# it's almost done, we now need to pop jormungandr (the python front end)
# the configuration is in the source/jormungandr/jormungandr/settings.py file
# default should be enough for the moment

#TODO honcho start with procfile

#TODO kraken

#TODO curl
echo "query on api endpoint: curl localhost:5000"
echo "$(curl localhost:5000)"

