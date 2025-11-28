******************************
How to build kraken and run it
******************************

Disclaimer
============
The following packages have been tested with **Debian 11**.
Later version of Debian or Ubuntu are not supported so far.

⚠️ If you are not using Debian 11, one can build within a docker container. ⚠️


Dependencies
============

#. C++

   * cmake
   * gcc (4.7 or newer)
   * libboost-all-dev (test, chrono, regex, system, serialization, date-time, thread, filesystem, iostreams, program-options)
   * libgeos++-dev
   * libgoogle-perftools-dev (tcmalloc)
   * liblog4cplus-dev
   * libosmpbf-dev (https://github.com/scrosby/OSM-binary)
   * libpqxx-dev (postgres)
   * libproj-dev
   * libprotobuf-dev
   * libssl-dev
   * libzmq3-dev

#. Python

   * python3.9
   * python3-pip
   * 2to3

   Each python module have a pip requirements.txt (and some of them a requirements_dev.txt) file that list it's dependencies.

   To install the dependencies for a module use `pip` (you can also wrap that in a virtualenv):

   ``pip install -r source/jormungandr/requirements_dev.txt``


#. Other

   * git
   * protobuf-compiler
   * RabbitMQ server
   * PostgreSQL (9.1+) and Postgis (2.0+)
   * Redis server

Build instruction
=================

#. Get the sources and the submodules of the project :

   ``git clone --recurse-submodules git@github.com:hove-io/navitia.git``

#. With CMake you can build outside of you source-tree :

   Let's bulid inside the `build_navitia` folder, next to the source:

   ``cmake -B build_navitia -S navitia/source``

   Note: it will build in release mode. If you want to compile it with debug symbols run

   ``cmake -DCMAKE_BUILD_TYPE=Debug  ...``

   A few of the Cmake options:

   * CMAKE_BUILD_TYPE=Release/Debug/RelWithDebInfo
   * SKIP_TESTS=ON - Compile without the test parts
   * STRIP_SYMBOLS=ON - Strip symbols within all code (Active -s option)

#. Compile

   ``make -C build_navitia -j${nproc}``
   Note: adjust -jX according to the number of cores you have

#. Run the tests

   ``make -C build_navitia test``

Testing
=======

#. Overview

.. image:: documentation/diagrams/simple_archi_data_view.png

#. Data configuration

   The data manager is called *ed*. It relies on GTFS and Open Street Map data centralized in a postgres database

#. Configure the postgres database

   #. Install the postgis extension http://postgis.net/docs/postgis_installation.html

      #. if your postgis version is newer than 2.0, just call create extention

         ``su postgres; #or the user you installed postgres with``

         ``psql -d yourdatabase -c "CREATE EXTENSION postgis;"``

      #. else call the init_db.sh script with a settings file.

         Copy the file source/script/settings.sh and update it with your parameters

         ``su postgres; #or the user you installed postgres with``

         ``cd source/scripts;``

         ``./init_db.sh your_settings.sh``


   #. Update the database scheme (with the usual user, not with 'postgres' anymore)

      ``cd source/sql``

      ``PYTHONPATH=. alembic -x dbname="postgresql://$db_user:$db_user_password@localhost/$db_name" upgrade head``

      If you prefer, you can also copy the alembic.ini config file and update the sqlalchemy.url field with you db connection string

      ``PYTHONPATH=. alembic -c your_alembic.ini upgrade head``

#. Get some GTFS data. For instance from http://data.navitia.io

   Import them using the gtfs2ed tool

#. Get some Open Street Map data. For instance from http://metro.teczno.com/

   Import them using the osm2ed tool

#. Once *ed* has been loaded with all the data, you have to export the data for *Kraken* with the ed2nav tool

   This step will generate a compressed nav file that can be given as input to *Kraken*

   ***Alternative to step 1 to 5*** : You can use *Eitri* as an alternative to generate the *.nav* file (cf. https://github.com/hove-io/navitia/tree/dev/source/eitri). This will setup the environment and call the right tool for you. Hasle-free Navitia file generator :)

#. Running the *Kraken* backend

   #. To run *Kraken*, you need to supply some parameters. You can give those parameters either via a file or via the command-line. By default you can take the documentation/examples/config/kraken.ini configuration file. The configuration file needs at least the path of the exported nav file and the zmq socket used to communicate with Jörmungandr. Run ``kraken --help`` to see the list of arguments

   #. Run *Kraken*. It should tell you what data it tries to load, and give some figures about the data

#. Running the *Jörmungandr* front-end. Note : this front-end is an API, and not oriented towards final users

   #. Edit if you want the Jormungandr.json file.

      Note: If you want to put the file elsewhere, you can change the INSTANCES_DIR variable

      example file : ::

        {
            "key": "some_region",
            "socket": "ipc:///tmp/default_kraken"
        }

   #. Give him the configuration file (by default it uses source/jormungandr/default_settings.py) and run it

      ``JORMUNGANDR_CONFIG_FILE=your_config.py FLASK_APP=jormungandr:app flask run``

   #. Grab a browser and open http://localhost:5000/v1/coverage/default_region

   #. Any trouble running Jormungandr see https://github.com/hove-io/navitia/blob/dev/source/jormungandr/readme.md
