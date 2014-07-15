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
# git clone https://github.com/CanalTP/navitia

navitia_du_user_password='navitia' #TODO recuperer un param
navitia_dir=`pwd $0`
gtfs_data_dir=$1  #TODO recup un param
osm_file=  #TODO recup un param

if [ "a$gtfs_data_dir" == "a" -o "a$osm_file" == "a" ]
then
    echo "no gtfs or osm file given, we'll take a test data set"
    #TODO recuperer des fichiers a coup de wget 
fi

run_dir=$navitia_dir/run
mkdir -p $run_dir

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
    apt-get install git g++ cmake liblog4cplus-dev libzmq-dev libosmpbf-dev libboost-all-dev libpqxx3-dev libgoogle-perftools-dev libprotobuf-dev python-pip libproj-dev protobuf-compiler libgeos-c1 
    
    apt-get install postgresql-9.3 postgresql-9.3-postgis-2.1 #Note: postgres 9.1 and postgis 2.0 would be enough, be postgis 2.1 is easier ton setup 
    
    
    # then you need to install all python dependencies: ::
    
    pip install -r /kraken/source/jormungandr/requirements.txt
    pip install -r /kraken/source/tyr/requirements.txt
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
su postgres -c "psql -c \"create user $db_owner;alter user $db_owner password '$navitia_du_user_password';\""
su postgres -c "psql -c \"create database $kraken_db_name owner $db_owner; \""
su postgres -c "psql -c \"create extension postgis; \" $kraken_db_name"

# Then you need to update it's scheme
# For that you can use a configuration file or just add environment variables
# There is an configuration file example in source/script/settings.sh
username=navitia server=localhost dbname=navitia PGPASSWORD='$navitia_du_user_password' $navitia_dir/source/scripts/update_db.sh


#Running
#=======

# ** filling up the database **
# we need to import the gtfs data
$navitia_build_dir/ed/fusio2ed -i $gtfs_data_dir --connection-string="host=localhost user=$db_owner password=$navitia_du_user_password"



