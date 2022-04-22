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
# the prerequisite the run that script is :
# - to have git and sudo installed
# - to have cloned the sources repository
#
# git clone https://github.com/hove-io/navitia
#
# - and to be in the cloned repository:
# cd navitia

# /!\ WARNING /!\
# the script needs the sudo (installed with dependencies) privileges for dependencies install and databases handling
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
scripts_dir="$(dirname $(readlink -f $0))"
navitia_dir="$scripts_dir"/..
ntfs_data_dir=
osm_file=

install_dependencies=1

clean_apt=

usage()
{
cat << EOF
usage: $0 options

This script setup a running navitia

only the password is mandatory:
 - if no dataset are given a default Paris one will be used
 - by default all dependencies are installed

Note that you must have sudo installed

OPTIONS:
   -h                  Show this message
   -p                  kraken database password
   -g                  gtfs directory
   -o                  osm file
   -n                  do not install dependencies
   -c                  if OS is Debian, clean the APT configuration (repository)
EOF
}

while getopts “hp:g:o:nc” OPTION
do
     case "$OPTION" in
         h)
             usage
             exit 1
             ;;
         p)
             kraken_db_user_password="$OPTARG"
             ;;
         g)
             ntfs_data_dir="$OPTARG"
             ;;
         o)
             osm_file="$OPTARG"
             ;;
         n)
             install_dependencies=
             ;;
         c)
             clean_apt=true
             ;;
         ?)
             usage
             exit 1
             ;;
     esac
done

if [ -z "$kraken_db_user_password" ]
then
    echo "no database password given, abort"
    exit 1
fi

#Be sure that basic dependencies are installed
sudo apt-get install -y unzip wget

if [ -z "$ntfs_data_dir" ] || [ -z "$osm_file" ]
then
    echo "no ntfs or osm file given, we'll take a default data set, Paris"

    echo "getting ntfs paris data from data.navitia.io"
    # Note, here is a link to a dataset of the paris region.
    # You can explore https://navitia.opendatasoft.com if you want another dataset
    wget -P /tmp https://navitia.opendatasoft.com/explore/dataset/fr-idf/files/dde578e47118b8c6f8885d75f18a504a/download/
    unzip -d /tmp/ntfs /tmp/index.html
    ntfs_data_dir=/tmp/ntfs

    echo "getting paris osm data from metro.teczno.com"
    wget -P /tmp http://osm-extracted-metros.s3.amazonaws.com/paris.osm.pbf
    osm_file=/tmp/paris.osm.pbf
fi

run_dir="$navitia_dir"/run
mkdir -p "$run_dir"

#Hack
#for convenience reason, some submodule links are in ssh (easier to push)
#it is however thus mandatory for the user to have a github access
#for this script we thus change the ssh links to https
sed -i 's,git\@github.com:\([^/]*\)/\(.*\).git,https://github.com/\1/\2,' "$navitia_dir"/.gitmodules

#we need to get the submodules
git submodule update --init --recursive

#========
#Building
#========
#
#First you need to install all dependencies.
#
#first the system and the c++ dependencies:
if [ -n "$install_dependencies" ]
then
    echo "** installing all dependencies"
    sudo apt-get install -y g++ cmake liblog4cplus-dev libzmq-dev libosmpbf-dev libboost-all-dev libgoogle-perftools-dev libprotobuf-dev python-pip libproj-dev protobuf-compiler libgeos-3.5.0 clang-format-6.0 2to3

    postgresql_package='postgresql-9.3'
    postgresql_postgis_package='postgis postgresql-9.3-postgis-2.1 postgresql-9.3-postgis-scripts'
    distrib=`lsb_release -si`
    version=`lsb_release -sr`
    pqxx_package='libpqxx3-dev'

    # Fix Ubuntu 15.04 package
    if [ "$distrib" = "Ubuntu" -a "$version" = "15.04" ]; then
      postgresql_package='postgresql-9.4'
      postgresql_postgis_package='postgis postgresql-9.4-postgis-2.1 postgresql-9.4-postgis-scripts'
    elif [ "$distrib" = "Ubuntu" -a "$version" = "16.04" ]; then
      postgresql_package='postgresql-9.5'
      postgresql_postgis_package='postgis postgresql-9.5-postgis-2.2 postgresql-9.5-postgis-scripts'

      # there is a bug in the liblog4cplus-dev in ubuntu 16,
      # https://bugs.launchpad.net/ubuntu/+source/log4cplus/+bug/1578970
      # we grab another version
      wget -P /tmp/ http://fr.archive.ubuntu.com/ubuntu/pool/universe/l/log4cplus/liblog4cplus-1.1-9_1.1.2-3.2_amd64.deb
      wget -P /tmp/ http://fr.archive.ubuntu.com/ubuntu/pool/universe/l/log4cplus/liblog4cplus-dev_1.1.2-3.2_amd64.deb
      sudo dpkg -i /tmp/liblog4cplus-1.1-9_1.1.2-3.2_amd64.deb  /tmp/liblog4cplus-dev_1.1.2-3.2_amd64.deb

      pqxx_package='libpqxx-dev'
    fi

    if [ "$distrib" = "Debian" ] && grep -q '^7\.' /etc/debian_version; then
            # on Debian, we must add the APT repository of PostgreSQL project
            # to have the right version of postgis
            # no magic stuff : https://wiki.postgresql.org/wiki/Apt#PostgreSQL_packages_for_Debian_and_Ubuntu
            apt_file='/etc/apt/sources.list.d/postgresql.list'
            sudo /bin/sh -c "echo 'deb http://apt.postgresql.org/pub/repos/apt/ wheezy-pgdg main' > $apt_file"
            sudo apt-get -y install wget ca-certificates
            wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -
            sudo apt-get update
    fi

    sudo apt-get install -y $postgresql_package $postgresql_postgis_package #Note: postgres 9.1 and postgis 2.0 would be enough, be postgis 2.1 is easier to setup

    sudo apt-get install -y $pqxx_package

    # then you need to install all python dependencies: ::

    sudo pip install -r "$navitia_dir"/source/jormungandr/requirements.txt
    sudo pip install -r "$navitia_dir"/source/tyr/requirements.txt
fi

#the build procedure is explained is the install documentation
echo "** building navitia"
navitia_build_dir="$navitia_dir"/build_release
mkdir -p "$navitia_build_dir" && cd "$navitia_build_dir"
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

# for the default build we give ownership of the base to a 'navitia' user, but you can do whatever you want here
encap=$(sudo -i -u postgres psql postgres -tAc "SELECT 1 FROM pg_roles WHERE rolname='$db_owner'")  # we check if there is already a user
if [ -z "$encap" ]; then
    sudo -i -u postgres psql -c "create user $db_owner;alter user $db_owner password '$kraken_db_user_password';"
else
    echo "user $db_owner already exists"
fi

if ! sudo -i -u postgres psql -l | grep -q "^ ${kraken_db_name}"; then
    sudo -i -u postgres createdb "$kraken_db_name" -O "$db_owner"
    sudo -i -u postgres psql -c "create extension postgis; " "$kraken_db_name"
else
    echo "db $kraken_db_name already exists"
fi

echo "** create temporary virtenv to migrate db"
if [ -d /tmp/venv_navitia_sql ]; then
    rm -rf /tmp/venv_navitia_sql
fi
virtualenv /tmp/venv_navitia_sql
. /tmp/venv_navitia_sql/bin/activate
pip install -r "$navitia_dir"/source/sql/requirements.txt

# Then you need to update it's scheme
# The database migration is handled by alembic
# You can edit the alembic.ini file if you want a custom behaviour (or give your own with the alembic -c option)
# you can give the database url either by setting the sqlalchemy.url parameter in the config file or by giving
# a -x dbname option
cd "$navitia_dir"/source/sql
PYTHONPATH=. alembic -x dbname="postgresql://$db_owner:$kraken_db_user_password@localhost/$kraken_db_name" upgrade head
deactivate
rm -rf /tmp/venv_navitia_sql
cd -

# Install jormungandr database and upgrade it's schema
# WARNING : default name is "jormungandr", so it should be the same in your SQLALCHEMY_DATABASE_URI on default_settings.py
if ! sudo -i -u postgres psql -l | grep -q "^ jormungandr"; then
    sudo -i -u postgres createdb jormungandr -O "$db_owner"
    sudo -i -u postgres psql -c "create extension postgis; " jormungandr
else
    echo "db jormungandr already exists"
fi

echo "** create temporary virtenv to upgrade jormun db"
if [ -d /tmp/venv_tyr ]; then
    rm -rf /tmp/venvtyr
fi
virtualenv /tmp/venv_tyr
. /tmp/venv_tyr/bin/activate
pip install -r "$navitia_dir"/source/tyr/requirements_dev.txt
cd "$navitia_dir"/source/tyr

PYTHONPATH=.:../navitiacommon/ TYR_CONFIG_FILE=default_settings.py ./manage_tyr.py db upgrade
deactivate
rm -rf /tmp/venv_tyr
cd -

#====================
# Filling up the data
#====================

# ** filling up the database **

## we need to import the ntfs data
"$navitia_build_dir"/ed/fusio2ed -i "$ntfs_data_dir" --connection-string="host=localhost user=$db_owner dbname=$kraken_db_name password=$kraken_db_user_password"

## we need to import the osm data
"$navitia_build_dir"/ed/osm2ed -i "$osm_file" --connection-string="host=localhost user=$db_owner dbname=$kraken_db_name password=$kraken_db_user_password"

## then we export the database into kraken's custom file format
"$navitia_build_dir"/ed/ed2nav -o "$run_dir"/data.nav.lz4 --connection-string="host=localhost user=$db_owner dbname=$kraken_db_name password=$kraken_db_user_password"

#========
# Running
#========

# * Kraken *
echo "** running kraken"
# we now need to pop the kraken

# Note we run Jormungandr and kraken in the same shell so the output might be messy

# We have to create the kraken configuration file
cat << EOF > "$run_dir"/kraken.ini
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
cd "$run_dir"
"$navitia_build_dir"/kraken/kraken &

kraken_pid=$!

# * Jormungandr *
echo "** running jormungandr"
# it's almost done, we now need to pop Jormungandr (the python front end)

# Jormungandr need to know how to call the kraken
# The configuration for that is handle by a repository where every kraken is referenced by a .ini file
mkdir -p "$run_dir"/jormungandr

# For our test we only need one kraken
cat << EOFJ > "$run_dir"/jormungandr/default.json
{
    "key": "default",
    "zmq_socket": "ipc:///tmp/default_kraken",
}
EOFJ

# the Jormungnandr configuration is in the source/jormungandr/jormungandr/default_settings.py file
# should be almost enough for the moment, we just need to change the location of the krakens configuration
sed "s,^INSTANCES_DIR.*,INSTANCES_DIR = '$run_dir/jormungandr'," "$navitia_dir"/source/jormungandr/jormungandr/default_settings.py > "$run_dir"/jormungandr_settings.py
#we also don't want to depend on the jormungandr database for this test
sed -i 's/DISABLE_DATABASE.*/DISABLE_DATABASE=False/' "$run_dir"/jormungandr_settings.py

pushd "$navitia_dir/source/jormungandr"
JORMUNGANDR_CONFIG_FILE="$run_dir"/jormungandr_settings.py PYTHONPATH="$navitia_dir/source/navitiacommon:$navitia_dir/source/jormungandr" FLASK_APP=jormungandr:app flask run &
popd

jormun_pid=$!

echo "That's it!"
echo "you can now play with the api"
echo "Note: you might have to wait a bit for the service to load the data"
echo "open another shell and try for example:"
echo "curl localhost:5000/v1/coverage/default/stop_areas"

#we block the script for the user to test the api
read -p "when you are finished, hit  a key to close kraken and jormungandr" n
clean_exit

# cleaning APT repository if -c option was specified
test -n "$clean_apt" && rm -f "$apt_file" && printf "Option -c was specified, removing %s \n" "$apt_file"
