.. image:: documentation/diagrams/logo_navitia_horizontal_fd_gris_250px.png
    :alt: navitia
    :align: center 

``(pronounce [navi-sia])``

.. image::  https://ci.navitia.io/buildStatus/icon?job=navitia_release
    :alt: build status
    :target: https://ci.navitia.io/job/navitia_release/


Presentation
============
Welcome to the Navitia repository. Navitia is a service providing:

#. multi-modal journeys computation

#. line schedules

#. next departures

#. explore public transport data

#. sexy things such as isochrones

The API can be found at http://api.navitia.io/

For more information see

* documentation http://doc.navitia.io
* main web site http://www.navitia.io
* playground http://canaltp.github.io/navitia-playground/
* twitter @navitia https://twitter.com/navitia
* google groups navitia https://groups.google.com/d/forum/navitia
* channel #navitia on irc.freenode.net

Getting started
===============

Want to test the API ?
----------------------

The easiest way to do this is a to go to `navitia.io <https://www.navitia.io/>`_.
`Signup <https://www.navitia.io/register/>`_, grab a token, read the `doc <http://doc.navitia.io>`_ and start using the API!

For a more friendly interface you can use the API through `navitia playground <http://canaltp.github.io/navitia-playground/>`_

Want to use you own datasets or infrastructure ?
------------------------------------------------

docker
~~~~~~
The easiest way to do have your own navitia is to use the navitia `docker-compose <https://github.com/CanalTP/navitia-docker-compose>`_.

fabric
~~~~~~
If you don't want to use the prebuild docker images you can use the `fabric scripts <https://github.com/CanalTP/fabric_navitia>`_ we use to deploy to api.navitia.io.

*WARNING:* Those scripts should be usable, but they are not meant to be completly generics and are designed for own servers architecture.

Use this only if the docker is not fit for your needs and if you are an experienced user :wink:

Want to dev in navitia ?
------------------------
if you want to build navitia, please refer to the `installation documentation <https://github.com/canaltp/navitia/blob/dev/install.rst>`_

You can also check the `automated build script <https://github.com/canaltp/navitia/blob/dev/build_navitia.sh>`_ which is meant as a step by step tutorial for compiling and using navitia with ubuntu 16.04

Code Organisation
=================
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

.. image:: documentation/diagrams/Navitia_simple_architecture.png

More information here: https://github.com/CanalTP/navitia/wiki/Architecture

Tools
======
#. Gcc (or clang) as the C++ compiler (g++)

#. Cmake for the build system

#. Python for the api

How to contribute
=================
Fork the github repo, create a new branch from dev, and submit your pull request!

Make sure to run the tests before submiting the pull request (`make test` in the build directory)

Are there many people contributing? Yes: https://www.youtube.com/watch?v=GOLfMTMGVFI

Alternatives?
=============
Navitia is written in python/c++, here are some alternatives:

* `OpenTripPlanner <https://github.com/opentripplanner/OpenTripPlanner/>`_ : written in java. More information here https://github.com/CanalTP/navitia/wiki/OpenTripPlanner-and-Navitia-comparison
* `rrrr <https://github.com/bliksemlabs/rrrr>`_ : the lightest one, written in python/c
* `Synthese <https://github.com/Open-Transport/synthese>`_ : a full stack, with CMS, written all in c++
* `Mumoro <https://github.com/Tristramg/mumoro>`_ : a R&D MUltiModal MUltiObjective ROuting algorithm
