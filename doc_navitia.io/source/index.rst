.. NAViTiA2 documentation master file, created by
   sphinx-quickstart on Fri Mar 30 16:58:49 2012.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to navitia.io API
=========================

*navitia.io* is an API to build cool stuff with public transport. We provide journeys computation, line schedules, easy access to anything related to public transport and sexy things such as isochrones.

Getting started
---------------

The easiest is probably to jump to `Examples`_ below.

At some point you will want to read:

.. toctree::
    :maxdepth: 1

    public_transport
    details
    API Reference <http://doc.navitia.io>
        
Our APIs are available at the following url: http://api.navitia.io.
        
Requests have the following structure : 
``http://api.navitia.io/v0/[region/]api.format?arg=val``. Have a look at the examples below to learn what API we provide and how to use them.

The region can be left out for requests that are based on coordinates. The format is one of json, xml or pb (for a result in Protocol Buffers).

There are no restriction in using our API. However, please don't make more than one request per second.

.. warning::
    This API is experimental. The parameters and responses are not definite as we will listen to your feedbacks to improve it.

    If you plan to build something successful, contact us to an access with more vitamines and even more support.

Let us know if you build something with our API. We will be happy hilight it on this page. The more feedback we get, the more cities you will get
and the more effort we will put to make the API durable.

Examples
--------

This chapter shows some usages with the minimal required arguments. However, this is not a reference and not all APIs nor arguments are shown.

A simple route computation
**************************

Let's find out how to get from the view point of the Golden Gate bridge to the Transamerica Pyramid in San Francisco the 18th of February at 08:00 AM.
We need to use the ``journeys`` API.


The coordinates of the view point are ``lon=-122.4752``, ``lat=37.80826`` and the coordinates of the Transamercia Pyramid are ``lon=-122.402770``, ``lat=37.794682``.
The coordinates are always in decimal degrees as WGS84 (also known as GPS coordinates).

The arguments are the following: 


* ``origin=coord:-122.4752:37.80826``
* ``destination=coord:-122.402770:37.794682``
* ``datetime=20130218T0800``

Hence, the complete URL : http://api.navitia.io/v0/journeys.json?origin=coord:-122.4752:37.80826&destination=coord:-122.402770:37.794682&datetime=20130418T0800.



A ``journeys`` request might return multiple journeys. Those journeys are said to be *equivalent*. For instance
a journey can be faster than an other but requires more changes or more walking.

This API has more options explained in the reference as :

* Forbid certain lines, routes or modes
* Latest departure


If you are wondering why the origin and destination have such a syntax, it's because we allow
to provide an address or a specific station as input. But we will see more about that later in the
section about entry points.


What stations are within 1000 meters
************************************

The ``proximity_list`` API finds any object within a certain radius as a crow flies.

Only the coordinates an ``uri`` is mandatory. Optionally you can select what object types to
return and change the radius. The URI must be a geographical coord or a stop point. Asking the stations arround
a network doesnot make much sense. Does it?

All stations arround the Transamerica Pyramid can be fetched with the following request : http://api.navitia.io/v0/proximity_list.json?uri=coord:-122.402770:37.794682.

What stations can be reached in the next 60 minutes
***************************************************

The ``isochrone`` API computes all routes from an origin to *all* stop points.
Only the ``origin`` and ``datetime`` must be given.


Compared to the ``journeys`` API, only one result is returned for every stop point: the earliest arrival.

Here is an example url :

http://api.navitia.io/v0/isochrone.json?origin=coord:-122.4752:37.80826&datetime=20130418T0800

Exploring the public transport objects
**************************************

*navitia* allows to request the objects and filter them by an other object. Every object has his own API,
but they all share the same filter argument. It is a very powerful requesting tool and its grammar is detailed in the :ref:`filter` section.


* Get all the stop_points of San Francisco : http://api.navitia.io/v0/sf/stop_points.json
* Get all the lines that go through the stop point with the uri ``stop_point:6526`` http://api.navitia.io/v0/sf/lines.json?filter=stop_point.uri=stop_point:6526
* Get all the routes that belong to the stop point with the uri ``stop_point:6526`` and to the line with the uri ``line:1045`` http://api.navitia.io/v0/sf/routes.json?filter=stop_point.uri=stop_point:6526+and+line.uri=line:1045

.. note::
    The results are paginated to avoid crashing your parser. The parameters to get the next page are within the result.

Getting help
------------

All available functions are documented on http://doc.navitia.io.

A mailing list is available to ask question : navitia@googlegroups.com

In order to report bug and make requests we created a `github navitia project <https://github.com/CanalTP/navitia/issues>`_ .

At last, we are present on IRC on the network Freenode, channel #navitia.

About…
------

About the data
**************

The street network is extracted from `OpenStreetMap <http://www.openstreetmap.org>`_ .

The public transport data are provided by networks that provide their timetables as open data.

About Canal TP
**************

`Canal TP <http://www.canaltp.fr>`_ is the editor of *navitia*. We are a company based in Paris, leader in
France for public transport information systems.

If you speak French, visit our `technical blog <http://labs.canaltp.fr>`_ .

