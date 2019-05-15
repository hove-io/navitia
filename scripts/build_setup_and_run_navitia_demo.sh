#!/bin/sh

scripts_dir="$(dirname $(readlink -f navitia))"
ntfs_data_dir=
osm_file=
skip_bina=
kraken_pid=
jormun_pid=

clean_exit()
{
 # kill the background job at the end
 echo "killing kraken (pid=$kraken_pid) and jormungandr (pid=$jormun_pid)";
 kill $kraken_pid
 kill -TERM $jormun_pid
 exit 0
}

usage()
{
cat << EOF
usage: $0 options

performs the following actions :

 - build kraken with cmake, make
 - download osm and ntfs (by default : Paris city in example)
 - load osm and ntfs into ed bdd
 - genrate data.nav.lz4 form bdd ed
 - create basic kraken configuration file and run
 - create basic jormun configuration files and run

by default, ED database used :
bdd name = navitia
user     = navitia
password = navitia

OPTIONS:
   -h        help
   -n        ntfs directory
   -o        osm file
   -s        skip binarization
EOF
}

while getopts “h:n:o:s” OPTION
do
     case "$OPTION" in
         h)
             usage
             exit 1
             ;;
         n)
             ntfs_data_dir="$OPTARG"
             ;;
         o)
             osm_file="$OPTARG"
             ;;
         s)
             skip_bina=true
             ;;
         ?)
             usage
             exit 1
             ;;
     esac
done

echo "** build, setup, and run navitia demo"

#===================================================
# Build Kraken
#===================================================
echo "** building navitia"
navitia_dir="$scripts_dir"/..
navitia_build_dir="$navitia_dir"/build_release
mkdir -p "$navitia_build_dir" && cd "$navitia_build_dir"
cmake -DCMAKE_BUILD_TYPE=Release ../source
make -j$(($(grep -c '^processor' /proc/cpuinfo)+1))

run_dir="$navitia_dir"/run
mkdir -p "$run_dir"

#===================================================
# Download OSM and GTFS data for example
#===================================================

if [ -z "$ntfs_data_dir" ] || [ -z "$osm_file" ] || [ ! -z "$skip_bina"]; then
    echo "** Download datas, we'll take a default data set, Paris"
    # Note, here is a link to a dataset of the paris region.
    # You can explore https://navitia.opendatasoft.com if you want another dataset

    ntfs_data_dir=/tmp/ntfs
    if [ ! -d /tmp/ntfs ]; then
        echo "** by default, getting ntfs paris data from data.navitia.io"
        wget -P /tmp https://navitia.opendatasoft.com/explore/dataset/fr-idf/files/dde578e47118b8c6f8885d75f18a504a/download/
        unzip -d /tmp/ntfs /tmp/index.html
    else
        echo "** ntfs paris data already exists in /tmp/ntfs "
    fi

    osm_file=/tmp/paris.osm.pbf
    if [ ! -f /tmp/paris.osm.pbf ]; then
        echo "** by default, getting paris osm data from metro.teczno.com"
        wget -P /tmp http://osm-extracted-metros.s3.amazonaws.com/paris.osm.pbf
    else
        echo "** paris.osm.pbf file already exists in /tmp "
    fi
fi

#===================================================
# Filling up the data
#===================================================
db_user_password='navitia'
db_name='navitia'
db_owner="navitia"

if [ -z "$skip_bina" ]; then

    # we need to import the gtfs data
    echo "** run {navitia_dir}/ed/fusio2ed"
    "$navitia_build_dir"/ed/fusio2ed -i "$ntfs_data_dir" --connection-string="host=localhost user=$db_owner dbname=$db_name password=$db_user_password"

    ## we need to import the osm data
    echo "** run {navitia_dir}/ed/osm2ed"
    "$navitia_build_dir"/ed/osm2ed -i "$osm_file" --connection-string="host=localhost user=$db_owner dbname=$db_name password=$db_user_password"

    ## then we export the database into kraken's custom file format
    echo "** run {navitia_dir}/ed/ed2nav"
    "$navitia_build_dir"/ed/ed2nav -o "$run_dir"/data.nav.lz4 --connection-string="host=localhost user=$db_owner dbname=$db_name password=$db_user_password"
else
    echo "** skip binarization"
fi

#===================================================
# Create basic Kraken configuration file
#===================================================
echo "** create {navitia_dir}/run/kraken/kraken.ini basic configuration file"
mkdir -p "$run_dir"/kraken
cat << EOF > "$run_dir"/kraken/kraken.ini
[GENERAL]
#file to load
database = ../data.nav.lz4
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

#===================================================
# Run Kraken
#===================================================
echo "** launch kraken engine in background"
cd "$run_dir"/kraken
if [ ! -L "$run_dir"/kraken/kraken ]; then
    ln -s "$navitia_build_dir"/kraken/kraken kraken
fi
./kraken &
kraken_pid=$!

#===================================================
# Create basic jormungandr configuration file
#===================================================
mkdir -p "$run_dir"/jormungandr
# For our test we only need one kraken
echo "** create {navitia_dir}/run/kraken.ini basic configuration file"
cat << EOFJ > "$run_dir"/jormungandr/default.json
{
    "key": "default",
    "zmq_socket": "ipc:///tmp/default_kraken"
}
EOFJ

echo "** create jormungandr virtenv {navitia_dir}/run/venv_jormungandr"
if [ ! -d "$run_dir"/jormungandr/venv_jormungandr ]; then
    virtualenv "$run_dir"/jormungandr/venv_jormungandr
    pip install -r "$navitia_dir"/source/jormungandr/requirements_dev.txt
fi
. "$run_dir"/jormungandr/venv_jormungandr/bin/activate
if [ ! -d "$run_dir"/jormungandr/venv_jormungandr ]; then
    pip install -r "$navitia_dir"/source/jormungandr/requirements_dev.txt
fi

#===================================================
# Run Jormungandr
#===================================================
echo "** launch jormungandr"
# the Jormungnandr configuration is in the source/jormungandr/jormungandr/default_settings.py file
# should be almost enough for the moment, we just need to change the location of the krakens configuration
sed "s,^INSTANCES_DIR.*,INSTANCES_DIR = '$run_dir/jormungandr'," "$navitia_dir"/source/jormungandr/jormungandr/default_settings.py > "$run_dir"/jormungandr/jormungandr_settings.py
#we also don't want to depend on the jormungandr database for this test
sed -i 's/DISABLE_DATABASE.*/DISABLE_DATABASE=False/' "$run_dir"/jormungandr/jormungandr_settings.py

JORMUNGANDR_CONFIG_FILE="$run_dir"/jormungandr/jormungandr_settings.py PYTHONPATH="$navitia_dir/source/navitiacommon:$navitia_dir/source/jormungandr" python "$navitia_dir"/source/jormungandr/jormungandr/manage.py runserver -d -r &
jormun_pid=$!


echo ""
echo ""
echo ""
echo "########  ########    ###    ########  ##    ##"
echo "##     ## ##         ## ##   ##     ##  ##  ##"
echo "##     ## ##        ##   ##  ##     ##   ####"
echo "########  ######   ##     ## ##     ##    ##"
echo "##   ##   ##       ######### ##     ##    ##"
echo "##    ##  ##       ##     ## ##     ##    ##"
echo "##     ## ######## ##     ## ########     ##"
echo ""
echo "That's it!"
echo "you can now play with the api"
echo "Note: you might have to wait a bit for the service to load the data"
echo "open another shell and try for example:"
echo "curl localhost:5000/v1/coverage/default/stop_areas"
echo ""
echo ""

#===================================================
# Hit enter to kill process
#===================================================
read -p "when you are finished, hit a key to close kraken and jormungandr" n
clean_exit
deactivate

