Welcome to navitia.io
=====================


An API to build cool stuff with public transport
-------------------------------------------------

We provide:

* journeys computation
* line schedules
* next departures
* exploration of public transport data
* sexy things such as isochrones

<aside class="notice">
Note:

We are gradually supporting more and more cities. If your city has open public transport data and is missing, drop us a note.
We will add it to navitia.io
</aside>


Getting started
---------------

The easiest is probably to jump to [Examples](#examples) below.

At some point you will want to read:

- [public_transport](#a-small-introduction-to-public-transportation-data)
- [integration](#navitia-documentation-v1-interface)

There are no restrictions in using our API. However, please don't make more than one request per second.

Let us know if you build something with our API, we will be happy to highlight it on this page. The more feedback we get, the more cities you will get
and the more effort we will put to make the API durable.


### Overview

*navitia* is a RESTful API that returns [JSON](http://en.wikipedia.org/wiki/Json) formated results.

Our APIs are available at the following url: <http://api.navitia.io/v1> (the latest API version is ``v1``).

The API has been built on the [HATEOAS model](http://en.wikipedia.org/wiki/HATEOAS) so the API should be quite self explanatory since the interactions are defined in hypermedia.

The different possible actions on a given API level are given in the ``links`` section of the response.

To provide additional arguments to the API, add them at the end of the query with the following syntax:  ``?arg1=val1&arg2=val2``.

Have a look at the examples below to learn what API we provide and how to use them.

.. note::
    The results are paginated to avoid crashing your parser. The parameters to get the next or previous page are within the ``links`` section of the result.


Examples
--------

This chapter shows some usages with the minimal required arguments. However, this is not a reference and not all APIs nor arguments are shown.


### A simple route computation

Let's find out how to get from the view point of the Golden Gate bridge to the Transamerica Pyramid in San Francisco the 18th of January at 08:00 AM.
We need to use the ``journeys`` API.


The coordinates of the view point are ``longitude = -122.4752``, ``latitude = 37.80826`` and the coordinates of the Transamercia Pyramid are ``longitude = -122.402770``, ``latitude = 37.794682``.
The coordinates are always in decimal degrees as WGS84 (also known as GPS coordinates). The coordinates are given to the API with the following format: ``longitute;latitude``.

The dates are given in the basic form of the ISO 8601 datetime format: ``YYYYMMDDTHHMM``.

The arguments are the following:


* ``from=-122.4752;37.80826``
* ``to=-122.402770;37.794682``
* ``datetime=20140118T0800``

Hence, the complete URL: <http://api.navitia.io/v1/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682&datetime=20140618T0800>.


A ``journeys`` request might return multiple journeys. Those journeys are said to be *equivalent*. For instance
a journey can be faster than an other but requires more changes or more walking.

This API has more options explained in the reference as:

* Forbid certain lines, routes or modes
  For example you can forbid the line 1030 and the cable car mode with the url:
  <http://api.navitia.io/v1/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682&datetime=20140618T0800&forbidden_uris[]=line:OSF:1030&forbidden_uris[]=commercial_mode:cablecar>

* Latest departure
  To get the latest departure, you can query for some journeys arriving before the end of the service
  <http://api.navitia.io/v1/journeys?from=-122.47787733594924;37.71696489300146&to=-122.41539259473815;37.78564348914185&datetime=20140613T0300&datetime_represents=arrival>

* You can also limit the maximum duration to reach the public transport system (to limit the walking/biking/driving parts):
  <https://api.navitia.io/v1/coverage/iledefrance/journeys?from=2.3865494;48.8499182&to=2.3643739;48.854&datetime=201406121000&max_duration_to_pt=1800>

* Enable biking, driving or use of bike sharing system
  For example you can allow bss (and walking since it's implicitly allowed with bss) at the departure:
  <https://api.navitia.io/v1/coverage/iledefrance/journeys?from=2.3865494;48.8499182&to=2.3643739;48.854&datetime=201406121000&max_duration_to_pt=1800&first_section_mode[]=bss>

  you can also allow walking and bike with:
  <https://api.navitia.io/v1/coverage/iledefrance/journeys?from=2.3865494;48.8499182&to=2.3643739;48.854&datetime=201406121000&max_duration_to_pt=1800&first_section_mode[]=bike&first_section_mode[]=walking>

  to allow a different fallback mode at the arrival mode, use ``last_section_mode[]``


### A quick journey in the API

*navitia* allows to dive into the public transport data.

To better understand how the API works let's ask the API the different main possibilities by simply querying the API endpoint: <http://api.navitia.io/v1/>

The ``links`` section of the answer contains the different possible interactions with the API.

As you can see there are several possibilities like for example ``coverage`` to navigate through the covered regions data or ``journeys`` to compute a journey.


Now let's see what interactions are possible with ``coverage``: <http://api.navitia.io/v1/coverage>

This request will give you:

* in the ``regions`` section the list of covered regions
* in the ``links`` section the list of possible interactions with them


In the ``links`` section there is for example this link: ``"href": "http://api.navitia.io/v1/coverage/{regions.id}/lines"``


This link is about lines (according to its ``rel`` attribute) and is templated which means that it needs additional parameters. The parameters are identified with the ``{`` ``}`` syntax.
In this case it needs a region id. This id can the found in the ``regions`` section. For example let's consider this region: ::

<pre>
{
    "start_production_date": "20140105",
    "status": "running",
    "shape": "POLYGON((-74.500997 40.344999,-74.500997 41.096999,-73.226 41.096999,-73.226 40.344999,-74.500997 40.344999))",
    "id": "ny",
    "end_production_date": "20140406"
}
</pre>

To query for the public transport lines of New York we thus have to call: <http://api.navitia.io/v1/coverage/ny/lines>


Easy isn't it?

We could push the exploration further and:

* get all the stop areas of the line with the uri ``line:BCO:Q10`` (the first line of the last request): <http://api.navitia.io/v1/coverage/ny/lines/line:BCO:Q10/stop_areas/>
* get the upcoming departures in the first stop area found with the last request (uri of the stop area ``stop_area:BCO:SA:CTP-BCO550123``):
  <http://api.navitia.io/v1/coverage/ny/stop_areas/stop_area:BCO:SA:CTP-BCO550123/departures/>
* get all the upcoming departures in the first stop area found in the last request, for the network mta:
  <http://api.navitia.io/v1/coverage/ny/stop_areas/stop_area:BCO:SA:CTP-BCO550123/networks/network:mta/departures/>


### What places have a name that start with 'tran'

The ``places`` API finds any object whose name matches the first letters of the query.

To find the objects that start with "tran" the request should be: <http://api.navitia.io/v1/coverage/ny/places?q=tran>

This API is fast enough to use it for autocompleting a user request.


### What places are within 1000 meters

The ``places_nearby`` API finds any object within a certain radius as a crow flies.
This API is not accessible from the main endpoint but has to be applied on a stop point, an address, some coordinates,...

All objects around the coordinates of the Transamerica Pyramid can be fetched with the following request: <http://api.navitia.io/v1/coverage/sf/coords/-122.402770;37.794682/places_nearby>

We could, in the same fashion, ask for the objects around a particuliar stop area (``stop_area:BCO:SA:CTP-BCO550123`` for example): <http://api.navitia.io/v1/coverage/ny/stop_areas/stop_area:BCO:SA:CTP-BCO550123/places_nearby>

Optionally you can select what object types to return and change the radius.


### What stations can be reached in the next 60 minutes


The API can computes *all* the reachable stop points from an origin within a given maximum travel duration.

All the stop points that can be reached from the Transamerica Pyramid can be fetched with the following request:
<http://api.navitia.io/v1/coverage/sf/coords/-122.402770;37.794682/journeys>

It returns for each destination stop point the earliest arrival and a link to the journey detail.


Getting help
------------

All available functions are documented on

- [integration](#integration)

A mailing list is available to ask question: navitia@googlegroups.com

In order to report bug and make requests we created a `github navitia project
<https://github.com/CanalTP/navitia/issues> .

At last, we are present on IRC on the network Freenode, channel #navitia.


About…
------


### About the service

If you plan to build something successful, contact us to get an access with more vitamins and even more support.

For the moment, the service is not hosted on a powerful server, so please don't make more than one request per second.

.. warning::
  Authentication is not required now, but will soon become (within april 2014).
  Please contact us for details.


### About the data


The street network is extracted from [OpenStreetMap](http://www.openstreetmap.org).

The public transport data are provided by networks that provide their timetables as open data.

Some data improvement are achieved by Canal TP.


### About Canal TP

[Canal TP](http://www.canaltp.fr) is the editor of *navitia*. We are a company based in Paris, leader in
France for public transport information systems.

If you speak French, visit our `technical blog <http://labs.canaltp.fr> .
