********
Navitia
********

Presentation
============
This is the Navitia repository.

Navitia is a service providing:

#. journeys computation

#. line schedules

#. next departures

#. explore public transport data

#. sexy things such as isochrones

The API can be found in http://api.navitia.io/

For more information see http://www.navitia.io/

Organisation
============
At the root of the repository, several directories can be found:

#. source: contains the navitia source code (c++ and python)

#. third_party: third party developped modules

#. documentation: all the navitia documentation

#. (debug|release): by convention, the build repositories

Architecture overview
=====================
Navitia is made of 3 main modules:

#. *Kraken* is the c++ core

#. *Jörmungandr* is the python frontend

#. *Ed* is the postgres database

*Kraken* and *Jörmungandr* communicate with each other through protocol buffer messages send by ZMQ.

Transportation data (in the `GTFS <https://developers.google.com/transit/gtfs/>`_ format) or routing data (from `OpenStreetMap <http://www.openstreetmap.org/>`_ for the moment) can be given to *Ed*. *Ed* produces a binary file used by *Kraken*.

.. image:: documentation/diagrams/simple_archi.png

Tools
======
#. Gcc as the C++ compiler (g++)

#. Cmake for the build system

#. Python for the api 

#. Doxygen and swagger for the automated documentation

Installation
============
For the installation procedure, please refer to the installation documentation : install.rst
