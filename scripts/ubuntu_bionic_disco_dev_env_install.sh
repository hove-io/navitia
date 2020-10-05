#!/bin/sh

# ********************************************************************
# Complete dev environment installation on Ubuntu bionic LTS.
# After the installation, you'll be able to :
# - Compile kraken
# - Run all integration ad Unit tests
# - Run docker (tyr + eitri) tests
# - Run binaries to fill ed database (fusio2ed, osm2ed, ...)
# - Run ed2nav binary to generate data.nav.lz4 form the ed database
# - Run eitri (the docker method) to build data.nav.lz4 with your own NTFS and Osm files
# ********************************************************************
echo "** launch dev env installation"

echo "** apt update"
sudo apt-get update

echo "** install dependencies pkg"
sudo apt-get install -y \
    build-essential \
    cmake \
    curl \
    clang-format-6.0 \
    protobuf-compiler \
    libosmpbf-dev \
    libboost-all-dev \
    libgoogle-perftools-dev \
    libprotobuf-dev \
    libproj-dev \
    libzmq3-dev \
    libpqxx-dev \
    libssl-dev \
    liblog4cplus-dev \
    libgeos-dev \
    rabbitmq-server \
    2to3

OS_VERSION="$(lsb_release -rs| cut -d '.' -f 1)"
echo "-- OS found: $(lsb_release -ds)"
if [ ${OS_VERSION} -eq 20 ]; then
    sudo apt-get install -y python2-dev
    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    sudo python2 get-pip.py
    rm get-pip.py
else
    sudo apt-get install -y python python-pip
fi

# Install docker.io for Tyr tests and eitri
echo "** install docker.io"
if [ -x "$(command -v docker)" ]; then
    echo "** docker.io is already installed"
else
    sudo apt-get install -y docker.io
    sudo usermod -aG docker $USER
    sudo systemctl restart docker
    sudo systemctl docker
fi

# Install postgre bdd
echo "** install postgresql & postgis extension"
sudo apt-get install -y postgresql postgis

# Install virtualenv
echo "** install virtualenv (pip)"
sudo -H pip install virtualenv

# Install pre-commit
echo "** install pre-commit (pip)"
sudo -H pip install pre-commit

# setup bdd
echo "** install postgres bdd"
scripts_dir="$(dirname $(readlink -f navitia))"
navitia_dir="$scripts_dir"/..
db_user_password='navitia'
db_name='navitia'
db_owner="navitia"

echo "** create navitia user "
encap=$(sudo -i -u postgres psql postgres -tAc "SELECT 1 FROM pg_roles WHERE rolname='$db_owner'")  # we check if there is already a user
if [ -z "$encap" ]; then
    sudo -i -u postgres psql -c "create user $db_owner;alter user $db_owner password '$db_user_password';"
else
    echo "** user $db_owner already exists"
fi

echo "** create navitia db "
if ! sudo -i -u postgres psql -l | grep -q "^ ${db_name}"; then
    sudo -i -u postgres createdb "$db_name" -O "$db_owner"
    sudo -i -u postgres psql -c "create extension postgis; " "$db_name"
else
    echo "** db $db_name already exists"
fi

echo "** create temporary virtenv to migrate bdd"
if [ -d /tmp/venv_navitia_sql ]; then
    rm -rf /tmp/venv_navitia_sql
fi
if [ ${OS_VERSION} -eq 20 ]; then
    virtualenv /tmp/venv_navitia_sql -p python2
else
    virtualenv /tmp/venv_navitia_sql -p python
fi
. /tmp/venv_navitia_sql/bin/activate
pip install -r "$navitia_dir"/source/sql/requirements.txt

echo "** migrate navitia bdd with alembic"
cd "$navitia_dir"/source/sql
PYTHONPATH=. alembic -x dbname="postgresql://$db_owner:$db_user_password@localhost/$db_name" upgrade head
deactivate
rm -rf /tmp/venv_navitia_sql

echo "** create jormun db "
if ! sudo -i -u postgres psql -l | grep -q "^ jormungandr"; then
    sudo -i -u postgres createdb jormungandr -O "$db_owner"
    sudo -i -u postgres psql -c "create extension postgis; " jormungandr
else
    echo "** db jormungandr already exists"
fi

echo "** create temporary virtenv to upgrade jormun bdd"
if [ -d /tmp/venv_tyr ]; then
    rm -rf /tmp/venvtyr
fi
if [ ${OS_VERSION} -eq 20 ]; then
    virtualenv /tmp/venv_tyr -p python2
else
    virtualenv /tmp/venv_tyr -p python
fi
. /tmp/venv_tyr/bin/activate
pip install -r "$navitia_dir"/source/tyr/requirements_dev.txt

echo "** migrate jormungandr bdd with alembic"
cd "$navitia_dir"/source/tyr
PYTHONPATH=.:../navitiacommon/ TYR_CONFIG_FILE=default_settings.py ./manage_tyr.py db upgrade
cd -
deactivate
rm -rf /tmp/venv_tyr

echo "**"
echo "**"
echo "**"
echo "** the installation is finished"
echo "**"
echo "**"
echo "                     WELCOME TO"
echo ""
echo "##    ##    ###    ##     ## #### ######## ####    ###"
echo "###   ##   ## ##   ##     ##  ##     ##     ##    ## ##"
echo "####  ##  ##   ##  ##     ##  ##     ##     ##   ##   ##"
echo "## ## ## ##     ## ##     ##  ##     ##     ##  ##     ##"
echo "##  #### #########  ##   ##   ##     ##     ##  #########"
echo "##   ### ##     ##   ## ##    ##     ##     ##  ##     ##"
echo "##    ## ##     ##    ###    ####    ##    #### ##     ##"
echo ""
echo "you are ready to push PRs"
echo ""
echo ""
echo "**********************************************"
echo "* !!! WARNING !!!"
echo "* You should probably log out or reboot because of docker."
echo "* Please test if docker runs correctly with :"
echo "* docker run hello-world"
echo "**********************************************"
echo ""
echo ""
echo "1. clone navitia"
echo "$ git clone https://github.com/CanalTP/navitia.git"
echo ""
echo "for convenience reason, some submodule links are in ssh (easier to push)"
echo "it is thus mandatory for the user to have a github access"
echo "However you can change the ssh links to https with :"
echo "$ sed -i 's,git\@github.com:\([^/]*\)/\(.*\).git,https://github.com/\1/\2,' .gitmodules"
echo ""
echo "2. compile navitia"
echo "$ mkdir build"
echo "$ cd build"
echo "$ cmake -DCMAKE_BUILD_TYPE=RELEASE navitia_source_dir"
echo "$ make -j"
echo ""
echo "3. install ED binaries [fusio2ed, ed2nav, ...] for eitri"
echo "$ make install (as a super user)"
echo "or"
echo "add the binaries location into your $PATH"
echo ""
echo "4. run tests"
echo "Please create your python virtualenv to launch tests with requirement_dev.txt"
echo "requirement_dev.txt are located in {navitia_source_dir}/jormungandr"
echo "Then launch,"
echo "$ make test"
echo ""
echo "5. Tyr tests (docker)"
echo "Please create your python virtualenv to launch tests with requirement_dev.txt"
echo "requirement_dev.txt are located in {navitia_source_dir}/tyr"
echo "Then launch,"
echo "$ make docker_test"
echo ""
