******************************
How to build kraken and run it
******************************

Dependencies
============

* gcc 4.7 or newer
* cmake
* log4cplus
* osmpbf
* boost
* zeromq
* libpqxx 3
* libgoogle-perftools-dev
* rabbitmq (>=3.1)
 pika (>=0.9.7)
* psycopg2

Build instruction
=================

We hope you got the source code from git.

1. Get the submodules: at the root of project :

   ``git submodule update --init``

2. Create a directory where everything will be built and eter it

   ``mkdir build``
   ``cd build``

3. Run cmake

   ``cmake ../source``

   Note: il will build in debug mode. If you want to compile it as a release run

   ``cmake -DCMAKE_BUILD_TYPE=Release ../source``

4. compile

   ``make -j4``

   Note: adjust -jX according to the number of cores you have

Testing
=======

#. Testing everything

   #. Get some GTFS data. For instance from http://data.navitia.io

   #. We suppose you create a data directory at the root of the kraken directory where you unziped the data

   #. Transform the data : ``./navimake -i ../data/ -o ../data/data.nav.lz4``

   #. Go to the navitia directory cd navitia

   #. Create the navitia.ini file with the following content ::

       [GENERAL]
       database=../../data/data.nav.lz4

   #. Run navitia  ./navitia it should tell you what data it tries to load, and give some figures about the data

#. Running the "frontend". Note : this frontend is an API, and not oriented towards final users

   #. Dependencies : python2 and the following python libraries werkzeug, shapely, zmq, protobuf

   #. Create the Jörmungandr.ini file with the following content: ::

       [instance]
       key = some_region
       socket = ipc:///tmp/default_navitia

   #. Run-it python2 navitia.py
   #. Grab a browser and open http://localhost:8088/some_region/stop_areas.txt
