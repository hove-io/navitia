#!/bin/sh


# **************************************
# Running Navitia in the blink of an eye
# **************************************
#
# You just need to blink slowly
#
# Here is a step by step guide for running navitia on a Ubuntu 14.04
#
#
# It's more an install guide but it can help as an out-of-a-box build script
# the prerequisite the run that script is to have cloned the sources repository
#
# git clone https://github.com/CanalTP/navitia
#
# and to be in the cloned repository:
# cd navitia

# /!\ WARNING /!\
# the script needs the sudo privileges for dependencies install and databases handling
# If used as an out of the box script be sure to read it beforehand
echo "!WARNING!"
echo "The script needs to install dependencies and update databases so it needs some privileges"
echo "It will thus prompt for your password"
echo "Make sure to review what the script is doing and check if you are ok with it"

#stop on errors
set -e

kraken_pid=
jormun_pid=
clean_exit()
{
#kill the background job at the end
 echo "killing kraken (pid=$kraken_pid) and jormungandr (pid=$jormun_pid)";
 kill $kraken_pid
 kill -TERM $jormun_pid
 exit 0
}

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
#First you need to install all dependencies. 
#
#first the system and the c++ dependencies: 
if [ $install_dependencies ]
then
    echo "** instaling all dependencies"
    sudo apt-get install -y git g++ cmake liblog4cplus-dev libzmq-dev libosmpbf-dev libboost-all-dev libpqxx3-dev libgoogle-perftools-dev libprotobuf-dev python-pip libproj-dev protobuf-compiler libgeos-c1 
    
    sudo apt-get install -y postgresql-9.3 postgresql-9.3-postgis-2.1 #Note: postgres 9.1 and postgis 2.0 would be enough, be postgis 2.1 is easier ton setup 
    
    # then you need to install all python dependencies: ::
    
    sudo pip install -r $navitia_dir/source/jormungandr/requirements.txt
    sudo pip install -r $navitia_dir/source/tyr/requirements.txt
fi

#the build procedure is explained is the install documentation
echo "** building navitia"
navitia_build_dir=$navitia_dir/release
mkdir -p $navitia_build_dir && cd $navitia_build_dir
cmake -DCMAKE_BUILD_TYPE=Release ../source
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1))


#=======================
#Setting up the database
#=======================
echo "** setting up the database"
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

if ! sudo su postgres -c "psql -l" | grep "^ $kraken_db_name "
then
sudo su postgres -c "createdb $kraken_db_name -O $db_owner"
sudo su postgres -c "psql -c \"create extension postgis; \" $kraken_db_name"
else 
echo "db $kraken_db_name already exists"
fi

# Then you need to update it's scheme
# For that you can use a configuration file or just add environment variables
# There is an configuration file example in source/script/settings.sh
username=$db_owner server=localhost dbname=$kraken_db_name PGPASSWORD=$kraken_db_user_password $navitia_dir/source/scripts/update_db.sh


#====================
# Filling up the data
#====================

# ** filling up the database **

## we need to import the gtfs data
$navitia_build_dir/ed/gtfs2ed -i $gtfs_data_dir --connection-string="host=localhost user=$db_owner password=$kraken_db_user_password"

## we need to import the osm data
$navitia_build_dir/ed/osm2ed -i $osm_file --connection-string="host=localhost user=$db_owner password=$kraken_db_user_password"

## then we export the database into kraken's custom file format
$navitia_build_dir/ed/ed2nav -o $run_dir/data.nav.lz4 --connection-string="host=localhost user=$db_owner password=$kraken_db_user_password"

#========
# Running
#========

# * Kraken * 
echo "** running kraken"
# we now need to pop the kraken

# Note we run Jormungandr and kraken in the same shell so the output might be messy

# We have to create the kraken configuration file
cat << EOF > $run_dir/kraken.ini
[GENERAL]
#file to load
database = data.nav.lz4
#ipc socket in default.ini file in the jormungandr instances dir
zmq_socket = ipc:///tmp/default_kraken
#number of threads
nb_threads = 1
#name of the instance
instance_name=default
[LOG]
log4cplus.rootLogger=DEBUG, ALL_MSGS

log4cplus.appender.ALL_MSGS=log4cplus::FileAppender
log4cplus.appender.ALL_MSGS.File=kraken.log
log4cplus.appender.ALL_MSGS.layout=log4cplus::PatternLayout
log4cplus.appender.ALL_MSGS.layout.ConversionPattern=[%D{%y-%m-%d %H:%M:%S,%q}] %b:%L [%-5p] - %m %n
EOF

# WARNING, for the moment you have to run it in the kraken.ini directory
cd $run_dir
$navitia_build_dir/kraken/kraken &

kraken_pid=$!

# * Jormungandr *
echo "** running jormungandr"
# it's almost done, we now need to pop Jormungandr (the python front end)

# Jormungandr need to know how to call the kraken
# The configuration for that is handle by a repository where every kraken is referenced by a .ini file
mkdir -p $run_dir/jormungandr

# For our test we only need one kraken
cat << EOFJ > $run_dir/jormungandr/default.ini 
[instance]
# name of the kraken
key = default
# zmq socket used to talk to the kraken, should be the same as the one defined by the zmq_socket param in kraken
socket = ipc:///tmp/default_kraken
EOFJ

# the Jormungnandr configuration is in the source/jormungandr/jormungandr/default_settings.py file
# should be almost enough for the moment, we just need to change the location of the krakens configuration
sed "s,^INSTANCES_DIR.*,INSTANCES_DIR = '$run_dir/jormungandr'," $navitia_dir/source/jormungandr/jormungandr/default_settings.py > $run_dir/jormungandr_settings.py

JORMUNGANDR_CONFIG_FILE=$run_dir/jormungandr_settings.py PYTHONPATH=$navitia_dir/source/navitiacommon:$navitia_dir/source/jormungandr python $navitia_dir/source/jormungandr/jormungandr/manage.py runserver -d -r & 

jormun_pid=$!

echo "That's it!"
echo "you can now play with the api"
echo "Note: you might have to wait a bit for the service to load the data"
echo "open another shell and try for example:"
echo "curl localhost:5000/v1/coverage/default/stop_areas"

#we block the script for the user to test the api
read -p "when you are finished, hit  a key to close kraken and jormungandr" n
clean_exit
