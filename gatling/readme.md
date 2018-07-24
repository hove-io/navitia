To test the load and performance of the different Navitia scenarios, the framework **Gatling** is used

How to run
===========

Gatling can be downloaded from the *gatling.io* page and is run with an executable script.
Here's an example of command to run the Navitia tests:

```bash
<PATH_TO>/bin/gatling.sh -df <PATH_TO>/navitia/gatling/data -sf <PATH_TO>/navitia/gatling/simulations
```

This command will show a list of simulations, based on the file found in the folder "simulations". In the command above, it is defined with the argument -sf (note that there can be default simulations found in the downloaded gatling folder).
A file with the arguments of the requests to run is provided with the argument -df.

Once executed, select the simulation number and the next steps can be skipped in our case:'simulation id' and 'description' (can be used to specify the commit id tested)

At the end of the simulation, a *stats html page* is generated.

The navitia simulations require to have running coverage (Jormungandr and Kraken) locally. The coverage name is define in the <ROOT_PROJECT>/gatling/data/<FILE>.csv (by default = fr-idf). To run correctly, You have to use the same coverage name.

Note: to reproduce a behaviour similar to the prod platform, it is necessary to launch several processes of **Jormun** using uwsgi.

uWsgi with Jormungandr
----------------------

**Install uWSGI on debian (and Ubuntu)**

```bash
sudo apt-get install uwsgi uwsgi-plugin-python
```

**uWsgi configuration file for Jormungandr**

You need to create a *uwsgi_config_file.ini* to run with Jormungandr app.

```ini
[uwsgi]
plugins = python
http-socket = :5000
wsgi-file = manage.py
callable = app
processes = 4
lazy-apps = True
gevent = 4
enable-threads = True
```

**Run Jormun with uWSGI**

Active the jormungandr virtual env and in *root_project/source/jormungandr/jormaungandr* run :

```bash
PYTHONPATH=/<ROOT_PROJECT>/source/navitiacommon:/<ROOT_PROJECT>/source/jormungandr JORMUNGANDR_CONFIG_FILE=default_settings.py uwsgi <PATH_TO>/uwsgi_config_file.ini
```

Run kraken with data reference
------------------------------

To test the load and performance, we always run with the same data set. Binarize the data with **eitri**, compile and run **kraken**.

How to add tests
================

The requests done for the simulations are '/journeys'. You can find in the data file (argument -df in the command above) the data needed for the requests. To add a test, add a line in the data file filling each argument as described in the first line of the file.
