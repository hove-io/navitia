.. image:: documentation/diagrams/logo_navitia_horizontal_fd_gris_250px.png
    :alt: navitia
    :align: center

=========
 Navitia
=========
``(pronounce [navi-sia])``

.. class:: no-web no-pdf

    Release branch: |last_release_build|

.. |last_release_build| image:: https://ci.navitia.io/job/navitia_release/badge/icon
    :target: https://ci.navitia.io/job/navitia_release/
    :alt: Last build


Presentation
============
Welcome to the Navitia repository !

Navitia is a webservice providing:

#. multi-modal journeys computation

#. line schedules

#. next departures

#. exploration of public transport data

#. search & autocomplete on places

#. sexy things such as isochrones


Approach
--------

| Navitia is an open-source web API, **initially** built to provide traveler information on urban
  transportation networks.
|
| Its main purpose is to provide day-to-day informations to travelers.
| Over time, Navitia has been able to do way more, *sometimes* for technical and debuging purpose
  *or* because other functional needs fit quite well in what Navitia can do *or* just because it was
  quite easy and super cool.
|
| Technically, Navitia is a HATEOAS_ API that returns JSON formated results.

.. _HATEOAS: http://en.wikipedia.org/wiki/HATEOAS


Who's who
----------

| Navitia is instanciated and exposed publicly through api.navitia.io_.
| Developments on Navitia are lead by Kisio Digital (previously CanalTP).
| Kisio Digital is a subsidiary of Keolis (itself a subsidiary of SNCF, French national railway company).

.. _api.navitia.io: http://api.navitia.io


More information
----------------

* main web site http://www.navitia.io
* playground http://canaltp.github.io/navitia-playground/
* integration documentation http://doc.navitia.io
* technical documentation https://github.com/CanalTP/navitia/tree/dev/documentation/rfc
* twitter @navitia https://twitter.com/navitia
* google groups navitia https://groups.google.com/d/forum/navitia
* channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org


Getting started
===============

Want to test the API ?
----------------------

| The easiest way to do this is a to go to `navitia.io <https://www.navitia.io/>`_.
| `Signup <https://www.navitia.io/register/>`_, grab a token, read the `doc <http://doc.navitia.io>`_
  and start using the API!

For a more friendly interface you can use the API through
`navitia playground <http://canaltp.github.io/navitia-playground/>`_ (no matter the server used).

Want to use you own datasets or infrastructure ?
------------------------------------------------

docker
~~~~~~

The easiest way to have your own navitia is to use the navitia
`docker-compose <https://github.com/CanalTP/navitia-docker-compose>`_.

fabric
~~~~~~

| If you don't want to use the prebuilt docker images you can use the
  `fabric scripts <https://github.com/CanalTP/fabric_navitia>`_ we use to deploy to api.navitia.io.
| :warning: *WARNING* :warning: Those scripts should be usable, but they are not meant to be completely
  generic and are designed for our own servers architecture.
| Use this only if the docker does not suit your needs and if you are an experienced user :wink:

Want to dev and contribute to navitia ?
---------------------------------------

If you want to build navitia, develop in it or read more about technical details please refer to
`CONTRIBUTING.md <https://github.com/canaltp/navitia/blob/dev/CONTRIBUTING.md>`_.

Curious of who's contributing? :play_or_pause_button: https://www.youtube.com/watch?v=GOLfMTMGVFI

Architecture overview
=====================
Navitia is made of 3 main modules:

#. *Kraken* is the c++ core (Heavy computation)

#. *Jörmungandr* is the python frontend (Webservice and lighter computation)

#. *Ed* is the postgres database (Used for preliminary binarization)

*Kraken* and *Jörmungandr* communicate with each other through protocol buffer messages send by ZMQ.

| Transportation data (in the `NTFS <https://github.com/CanalTP/ntfs-specification/blob/master/readme.md>`_,
  or `GTFS <https://developers.google.com/transit/gtfs/>`_ format) or routing data
  (mainly from `OpenStreetMap <http://www.openstreetmap.org/>`_ for the moment) can be given to *Ed*.
| *Ed* produces a binary file used by *Kraken*.

.. image:: documentation/diagrams/Navitia_simple_architecture.png

More information here: https://github.com/CanalTP/navitia/wiki/Architecture

Alternatives?
=============
Navitia is written in C++ / python, here are some alternatives:

* | `OpenTripPlanner <https://github.com/opentripplanner/OpenTripPlanner/>`_ : written in java.
  | More information here https://github.com/CanalTP/navitia/wiki/OpenTripPlanner-and-Navitia-comparison.
* `rrrr <https://github.com/bliksemlabs/rrrr>`_ : the lightest one, written in python/c
* `Synthese <https://github.com/Open-Transport/synthese>`_ : full stack, with CMS, written all in C++
* `Mumoro <https://github.com/Tristramg/mumoro>`_ : a R&D MUltiModal MUltiObjective ROuting algorithm
