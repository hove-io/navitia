Welcome to navitia.io
=====================

The API to build cool stuff with public transport

Overview
--------

*navitia* is a RESTful API which provide:

* journeys computation
* line schedules
* next departures
* exploration of public transport data
* sexy things such as isochrones

<aside class="success">
    We are gradually supporting more and more cities. If your city has open public transport data and is missing, drop us a note.
    We will add it to navitia.io
</aside>

Let us know if you build something with our API, we will be happy to highlight it on this page. The more feedback we get, the more cities you will get
and the more effort we will put to make the API durable.

Our APIs are available at the following url: <http://api.navitia.io/v1> (the latest API version is ``v1``).

The API has been built on the [HATEOAS model](http://en.wikipedia.org/wiki/HATEOAS) so the API should be quite self explanatory since the interactions are defined in hypermedia.

Have a look at the examples below to learn what services we provide and how to use them.

Getting started
---------------

#### First,
get a token here <http://navitia.io/register/>

#### Second,
use the token : if you use a web browser, you only have to **paste it in the user area**,
with **no password**. 
Or, in a simplier way, you can add your token in the address bar like :


<aside class="success">
<b>https://01234567-89ab-cdef-0123-456789abcdef@api.navitia.io/v1/coverage/fr-idf/networks</b>
</aside>

See [authentication](#authentication) section to find out more details on **how to use your token**.

#### Then,
use the API ! The easiest is probably to jump to [Examples](#examples) below.

At some point you will want to read:

- [transport public lexicon](#lexicon)
- [integration](#v1 interface)


Examples
--------

This chapter shows some usages with the minimal required arguments. However, this is not a reference and not all APIs nor arguments are shown.


### A simple route computation

Let's find out how to get from the view point of the Golden Gate bridge to the Transamerica Pyramid in San Francisco.
We need to use the ``journeys`` API.


The coordinates of the view point are ``longitude = -122.4752``, ``latitude = 37.80826`` and the coordinates of the Transamercia Pyramid are ``longitude = -122.402770``, ``latitude = 37.794682``.
The coordinates are always in decimal degrees as WGS84 (also known as GPS coordinates). The coordinates are given to the API with the following format: ``longitute;latitude``.

The arguments are the following:


* ``from=-122.4752;37.80826``
* ``to=-122.402770;37.794682``
Hence, the complete URL: <http://api.navitia.io/v1/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682>.

<aside class="success">
A ``journeys`` request might return multiple journeys. Those journeys are said to be equivalent. For instance
a journey can be faster than an other but requires more changes or more walking.
</aside>

This API has more options explained in the reference as:

* The dates are given in the basic form of the ISO 8601 datetime format: ``YYYYMMDDTHHMM``. 
  For example, if you want to compute a journey on friday, April 22 ``datetime=20160422T0800``
  <http://api.navitia.io/v1/journeys?from=-122.47787733594924;37.71696489300146&to=-122.41539259473815;37.78564348914185&datetime=20160422T0800>

* Latest departure
  To get the latest departure, you can query for some journeys arriving before the end of the service
  <http://api.navitia.io/v1/journeys?from=-122.47787733594924;37.71696489300146&to=-122.41539259473815;37.78564348914185&datetime=20160422T2359&datetime_represents=arrival>

* You can also change the [traveler profile](#traveler-type) (to adapt the walking/biking/driving parts and comfort of journeys):
  <http://api.navitia.io/v1/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682&traveler_type=slow_walker>

* Forbid certain lines, routes or modes
  For example you can forbid the line 1 and the cable car mode with the url:
  <http://api.navitia.io/v1/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682&forbidden_uris[]=line:OSF:10867&forbidden_uris[]=commercial_mode:cablecar>

* Enable biking, driving or use of bike sharing system on Paris area
  For example you can allow bss (and walking since it's implicitly allowed with bss) at the departure:
  <https://api.navitia.io/v1/journeys?from=2.3865494;48.8499182&to=2.3643739;48.854&first_section_mode[]=bss&first_section_mode[]=walking&first_section_mode[]=bike>


### A quick exploration of the API

*navitia* allows to dive into the public transport data.

To better understand how the API works let's ask the API the different main possibilities by simply querying the API endpoint: <http://api.navitia.io/v1/>

The ``links`` section of the answer contains the different possible interactions with the API.

As you can see there are several possibilities like for example [coverage](#coverage) to navigate through the covered regions data or [journeys](#journeys) to compute a journey.


Now let's see what interactions are possible with ``coverage``: <http://api.navitia.io/v1/coverage>

This request will give you:

* in the ``regions`` section the list of covered regions
* in the ``links`` section the list of possible interactions with them


In the ``links`` section there is for example this link: ``"href": "http://api.navitia.io/v1/coverage/{regions.id}/lines"``


This link is about lines (according to its ``rel`` attribute) and is templated which means that it needs additional parameters.<br>
The parameters are identified with the ``{`` ``}`` syntax.
In this case it needs a region id. This id can the found in the ``regions`` section. For example let's consider this region:

``` json
{
    "start_production_date": "20140105",
    "status": "running",
    "shape": "POLYGON((-74.500997 40.344999,-74.500997 41.096999,-73.226 41.096999,-73.226 40.344999,-74.500997 40.344999))",
    "id": "us-ny",
    "end_production_date": "20140406"
}
```

To query for the public transport lines of New York we thus have to call: <http://api.navitia.io/v1/coverage/us-ny/lines>


Easy isn't it?

We could push the exploration further and:

- Where am I? (WGS 84 coordinates)
    - <https://api.navitia.io/v1/coord/2.377310;48.847002>
    - I'm on the "/fr-idf" coverage, at "20, rue Hector Malot in Paris, France"
- Which services are available on this coverage? Let's take a look at the links at the bottom of this stream
    - <https://api.navitia.io/v1/coverage/fr-idf>
- Networks available? (see what [network](#network) is)
    - <https://api.navitia.io/v1/coverage/fr-idf/networks>
    - pwooo, many networks on this coverage ;)
- Is there any Metro lines or networks?
    - there is an api for that. See [pt_objects](#pt-objects)
    - <https://api.navitia.io/v1/coverage/fr-idf/pt_objects?q=metro>
    - Response contain one network, one mode, and many lines
- Let's try some filtering (see [ptreferential](#ptreferential))
    - filter on the specific metro network ("id": "network:OIF:439" extracted from last request)
    - <https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/>
    - physical modes managed by this network
    - <https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/physical_modes>
    - metro lines
    - <https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/physical_modes/physical_mode:Metro/lines>
- By the way, what stuff are close to me?
    - <https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/places_nearby>
    - or <https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/lines>
    - or <https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/stop_schedules>
    - or ...



### What places have a name that start with 'eiff'

The [/places](#places) API finds any object whose name matches the first letters of the query.

To find the objects that start with "tran" the request should be: <http://api.navitia.io/v1/coverage/fr-idf/places?q=eiff>

This API is fast enough to use it for autocompleting a user request.


### What places are within 1000 meters

The [/places_nearby](#places-nearby) API finds any object within a certain radius as a crow flies.
This API is not accessible from the main endpoint but has to be applied on a stop point, an address, some coordinates,...

All objects around the coordinates of the Transamerica Pyramid can be fetched with the following request: <http://api.navitia.io/v1/coverage/us-ca/coords/-122.402770;37.794682/places_nearby>

We could, in the same fashion, ask for the objects around a particuliar stop area (``stop_area:OSF:SA:CTP4025`` for example): <http://api.navitia.io/v1/coverage/us-ca/stop_areas/stop_area:OSF:SA:CTP4025/places_nearby>

Optionally you can select what object types to return and change the radius.


### What stations can be reached in the next 20 minutes

The API can computes *all* the reachable stop points from an origin within a given maximum travel duration.

All the stop points that can be reached from the Transamerica Pyramid can be fetched with the following request:
<http://api.navitia.io/v1/coverage/us-ca/coords/-122.402770;37.794682/journeys?max_duration=1200>

It returns for each destination stop point the earliest arrival and a link to the journey detail.


Getting help
------------

All available functions are documented in [integration part](#navitia-documentation-v1-interface)

A mailing list is available to ask question: <a href="mailto:navitia@googlegroups.com">navitia@googlegroups.com</a>

In order to report bug and make requests we created a github navitia project
<https://github.com/CanalTP/navitia/issues>.

Stay tuned on twitter @navitia.

At last, we are present on IRC on the network <a href="https://webchat.freenode.net/">Freenode</a>, channel <b>#navitia</b>.

