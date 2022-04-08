# Setting up Navitia on OSX using Vagrant

## Requirements

* Vagrant
* Virtualbox
* Storage: >10 GB
* Memory: > 6GB
* Multi-core
* Vagrantfile for Navitia
* VM requirements settings

Check that Vagrantfile provides enough memory and CPU:
```
config.vm.provider "virtualbox" do |vb|
  vb.memory = "8192"
  vb.cpus = "4"
end
```



## Installation

**HOST**

Setup folders
```sh
mkdir dev
mkdir dev/navitia_dev
mkdir dev/vagrant
cp /path/to/Vagrantfile dev/vagrant
```

Download Virtualbox Guest additions ISO image file (do not open) : [download.virtualbox.org/virtualbox/5.0.16/VBoxGuestAdditions_5.0.16.iso](download.virtualbox.org/virtualbox/5.0.16/VBoxGuestAdditions_5.0.16.iso)

Setup VM
```sh
cd dev/vagrant
vagrant up
vagrant halt
```

In VirtualBox (make sure VM is not running!):
Add virtual drive > optical drive > Load VirtualBox Guest Additions ISO

You can now access guest VM in VAGRANT using:
```sh
cd dev/vagrant
vagrant up
vagrant ssh
```


**VAGRANT**

Install additional dependencies in the VM:
```sh
sudo apt-get update
sudo apt-get install -y linux-headers-$(uname -r)
sudo apt-get install -y build-essential

sudo mount /dev/cdrom /mnt
sudo /mnt/VBoxLinuxAdditions.run
exit
```


**HOST**

Restart guest VM:
```sh
vagrant halt
vagrant up
```

Clone source files (use ssh instead of http if you have a github account, no gitmodule fix is needed):
```sh
cd dev/navitia_dev
mkdir source build
cd source
git clone https://github.com/hove-io/navitia.git
```

If you don't use a github account, before submodule init:
Fix /dev/navitia_dev/source/navitia/.gitmodules with correct url !
```sh
sed s#git@github.com:#https://github.com/#g /dev/navitia_dev/source/navitia/.gitmodules
```

Below is what it should look like:
```
[submodule "source/utils"]
path = source/utils
url = https://github.com/hove-io/utils.git

[submodule "source/third_party/osmpbfreader"]
path = source/third_party/osmpbfreader
url = https://github.com/hove-io/libosmpbfreader.git

[submodule "source/third_party/SimpleAmqpClient"]
path = source/third_party/SimpleAmqpClient
url = https://github.com/hove-io/SimpleAmqpClient.git

[submodule "source/third_party/librabbitmq-c"]
path = source/third_party/librabbitmq-c
url = https://github.com/alanxz/rabbitmq-c.git

[submodule "source/linenoise"]
path = source/linenoise
url = https://github.com/antirez/linenoise.git

[submodule "source/chaos-proto"]
path = source/chaos-proto
url = https://github.com/hove-io/chaos-proto.git

[submodule "source/navitia-proto"]
path = source/navitia-proto
url = https://github.com/hove-io/navitia-proto.git
```

Update submodules:
```sh
cd navitia
git submodule update --init --recursive
```


**VAGRANT**

Install dependancies:
```sh
sudo apt-get install -y virtualenvwrapper g++ cmake liblog4cplus-dev libzmq-dev libosmpbf-dev libboost-all-dev libpqxx3-dev libgoogle-perftools-dev libprotobuf-dev python-pip libproj-dev protobuf-compiler libgeos-c1 git
```

Run huge build (takes long time):
```sh
mkdir -p /navitia_dev/build/release
cd /navitia_dev/build/release
cmake -DCMAKE_BUILD_TYPE=Release ../../source/navitia/source

# run make in multiple steps, this will ensure we can move on with
# the rest of the install without waiting for the full compile.
# make sure the component you use finishes compiling first for data processing!
make -j4 fusio2ed && make gtfs2ed && make -j4 osm2ed && make -j4 ed2nav

# now build kraken and generate python protobuf files (in navitiacommon)
make -j4 kraken && make protobuf_files

# then if you want to compile the rest (mostly for unit testing)
make -j4
```

While long compile is running, you can open another tab (still using vagrant):

Add the following lines to ~/.bashrc:
```
source /usr/share/virtualenvwrapper/virtualenvwrapper.sh

WORKON_HOME="~/.venv"
```

Make venv dir for python modules:
```sh
mkdir ~/.venv
```

Setup Jormungandr:
```sh
cd /navitia_dev/source/navitia/source/jormungandr
mkvirtualenv jormungandr
workon jormungandr
pip install pip -U
pip install setuptools -U

pip install -r requirements.txt -U
pip install -r requirements_dev.txt -U
```

Setup Tyr:
```sh
cd /navitia_dev/source/navitia/source/tyr
mkvirtualenv tyr
workon tyr
pip install pip -U
pip install setuptools -U

pip install -r requirements.txt -U
pip install -r requirements_dev.txt -U
```

Setup PostgreSQL (PostGIS) database:
You may also set up the database on the HOST (under /navitia_dev/postgresql/9.4/main) and connect to it in the VM (if you run into storage issues)
```sh
sudo apt-get install -y postgresql-9.4 postgis postgresql-9.4-postgis-2.1 postgresql-9.4-postgis-scripts alembic
sudo -i -u postgres

psql
CREATE USER navitia;
ALTER ROLE navitia WITH CREATEDB;
ALTER USER navitia WITH ENCRYPTED PASSWORD 'navitia';
CREATE DATABASE nav_tfl OWNER navitia;
\q

psql -d nav_tfl -c "CREATE EXTENSION postgis;"
exit

cd /navitia_dev/source/navitia/source/sql/
workon tyr
pip install -r requirements.txt -U
PYTHONPATH=. alembic -x dbname="postgresql://navitia:navitia@localhost/nav_tfl" upgrade head
```

Install RabbitMQ Server:
```sh
sudo apt-get install -y rabbitmq-server
```


**HOST**

Download dataset from OSM:
```sh
cd dev/navitia_dev
mkdir -p data/tfl
cd data/tfl
mkdir ntfs
mkdir osm

# download OpenStreetMap for Greater London from geofabrik
cd osm
curl -O http://download.geofabrik.de/europe/great-britain/england/greater-london-latest.osm.pbf

#Go back to data/tfl and put the NTFS files (zip) in that directory.
#Unzip NTFS (all txt files) into the data/tfl/ntfs folder
cd ../tfl
unzip tfl_ntfs.zip -d ntfs/
```

**VAGRANT**

Configure jormungandr:
```sh
mkdir -p /navitia_dev/run/jormungandr
cd /navitia_dev/run/jormungandr
```

In that directory create default.json that should look like:
```
{
     "key": "navitia",
     "zmq_socket": "ipc:///tmp/default_kraken"
}
```

Create jormungandr_settings.py:
```sh
cd /navitia_dev/run

sed "s,^INSTANCES_DIR.*,INSTANCES_DIR = '/navitia_dev/run/jormungandr'," "/navitia_dev/source/navitia/source/jormungandr/jormungandr/default_settings.py" > "/navitia_dev/run/jormungandr_settings.py"
sed -i 's/DISABLE_DATABASE.*/DISABLE_DATABASE=True/' "/navitia_dev/run/jormungandr_settings.py"
```

Edit jormungandr_settings.py (at the end of the file):
```
GRAPHICAL_ISOCHRONE = True (original: False)
```

Copy kraken.ini file:
```sh
cp /navitia_dev/source/navitia/documentation/examples/config/kraken.ini /navitia_dev/data/tfl
```

Edit kraken.ini:
Add the following in [GENERAL] section, before [LOG]
```
instance_name = navitia

[BROKER]
host = localhost
port = 5672
username = guest
password = guest
vhost = /
exchange = navitia
rt_topics = chaos_contrib

[CHAOS]
database = host=localhost user=navitia password=navitia dbname=chaos

```

You can tweak db params (for faster performance), edit postgreSQL config:
```sh
sudo vi /etc/postgresql/9.4/main/postgresql.conf
```
Set the following params for faster performance:
```
work_mem = 10MB
checkpoint_segments = 10
```
Restart postgres:
```sh
sudo /etc/init.d/postgresql restart
```

Launch the data processing (make sure fusio2ed/gtfs2ed, osm2ed and ed2nav are compiled!) + monitor RAM usage!
```sh
cd /navitia_dev/build/release

./ed/fusio2ed -i /navitia_dev/data/tfl/ntfs/ --connection-string="host=localhost user=navitia password=navitia dbname=nav_tfl"
# or: ./ed/gtfs2ed -i /navitia_dev/data/tfl/gtfs/ --connection-string="host=localhost user=navitia password=navitia dbname=nav_tfl"

./ed/osm2ed -i /navitia_dev/data/tfl/osm/greater-london-latest.osm.pbf --connection-string="host=localhost user=navitia password=navitia dbname=nav_tfl"

./ed/ed2nav -o /navitia_dev/data/tfl/data.nav.lz4 --connection-string="host=localhost user=navitia password=navitia dbname=nav_tfl"
```

Now we need to wait for kraken compilation.

Run Kraken:
```sh
cd /navitia_dev/data/tfl
/navitia_dev/build/release/kraken/kraken &
```

Set up jormungandr database:
```sh
cd /navitia_dev/source/navitia/source/tyr
workon tyr
PYTHONPATH=../navitiacommon:. TYR_CONFIG_FILE=dev_settings.py ./manage_tyr.py db upgrade
```

Run jormungandr:
```sh
cd /navitia_dev/source/navitia/source/jormungandr
workon jormungandr
JORMUNGANDR_CONFIG_FILE="/navitia_dev/run/jormungandr_settings.py" PYTHONPATH="/navitia_dev/source/navitia/source/navitiacommon:/navitia_dev/source/navitia/source/jormungandr" python "/navitia_dev/source/navitia/source/jormungandr/jormungandr/manage.py" runserver -d -r -t 0.0.0.0 -p 5001
```

Navitia API is now running at localhost:5001!


Setting up Chaos (see docs in Github): [https://github.com/hove-io/Chaos](https://github.com/hove-io/Chaos)
```sh
cd /navitia_dev/source
git clone https://github.com/hove-io/Chaos.git

cd Chaos
mkvirtualenv chaos
pip install -r requirements.txt

# ! change .gitmodules url if using http instead of ssh !

git submodule update --init --recursive
./setup.py build_pbf
```

Create the chaos database:
```sh
sudo apt-get install postgresql libpq-dev
sudo -i -u postgres

# Create a user
createuser -P navitia (password "navitia")

# Create database
createdb -O navitia chaos

# Create database for tests
createdb -O navitia chaos_testing
```


Edit Chaos/Procfile that should look like:
```
web: ./manage.py runserver -t 0.0.0.0
```

Update NAVITIA_URL in Chaos/default_settings.py:
```
NAVITIA_URL = 'http://localhost:5001'
```

Chaos is now available on port 5000!



## Feeding CHAOS

Send HTTP POST request with header X-Customer-Id: chaos-customer to http://localhost:5000/channels
```sh
curl -XPOST http://localhost:5000/channels -H "X-Customer-Id: chaos-customer" -H "Content-Type: application/json" -d '{"name": "foo", "max_size": 500, "content_type": "text/type", "types": ["web","mobile"]}'
```


post to /causes:
```sh
curl -XPOST http://localhost:5000/causes -H "X-Customer-Id: chaos-customer" -H "Content-Type: application/json" -d '{"wordings": [{"key": "aa", "value": "bb"}, {"key": "dd", "value": "cc"}]}'
```

post to /severities:
```sh
curl -XPOST http://localhost:5000/severities -H "X-Customer-Id: chaos-customer" -H "Content-Type: application/json" -d '{"wordings": [{"key": "test", "value": "foo"}]}'
```

post to /disruptions:
```sh
curl -XPOST http://localhost:5000/disruptions -H "X-Customer-Id: chaos-customer" -H "Content-Type: application/json" -H "X-Coverage: navitia" -H "Authorization: foo" -d <use-snippet-below>
```

In the data snippet below, edit the ids of cause, channels, severities with the ids in responses from previous calls:
```
{
  "reference": "foo",
  "contributor": "chaos_contrib",
  "cause": {
    "id": "0d23369e-5022-11e6-90ae-080027f686bf"
  },
  "publication_period": {
    "begin": "2016-01-24T10:35:00Z",
    "end": "2016-09-24T23:59:59Z"
  },
  "impacts": [{
    "objects": [{
      "type": "network",
      "id": "network:OUK:LE"
    }],
    "messages": [{
      "text": "message_text",
      "channel": {
        "id": "7347079e-5021-11e6-90ae-080027f686bf"
      }
    }],
    "severity": {
      "id": "45750482-5022-11e6-90ae-080027f686bf"
    },
    "application_periods": [{
      "begin": "2016-06-29T16:52:00Z",
      "end": "2016-07-22T02:15:00Z"
    }, {
      "begin": "2016-04-29T16:52:00Z",
      "end": "2016-09-22T02:15:00Z"
    }]
  }]
}
```


## Troubleshooting

1. Error mount: unknown filesystem type 'vboxsf'
Go into /dev/vagrant/Vagrantfile and comment out the rows with config.vm.synced_folder (this will only work after we install the Guest Additions).
After you install the Guest additions, stop the VM, uncomment and restart.

2. Make sure to set locale otherwise postgres will give an error! (if you see a locale warning while setting up postgres, this is it)
To set your locale, run `sudo dpkg-reconfigure locales`, scroll down the list, select yours (for example en_GB.UTF-8) and select OK.

3. `[ERROR] - Data loading failed: Unable to connect to chaos database: FATAL:  database "chaos" does not exist`
Edit kraken.ini to remove the config sections [BROKER] and [CHAOS] to test that it works without it, or create a proper chaos database.
