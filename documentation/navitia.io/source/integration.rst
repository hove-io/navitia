Navitia documentation: v1 interface 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. contents:: Index

Overview
========

This document describes how to call navitia via the v1 interface, and the returned resources.
You can read our lexicon at https://github.com/OpenTransport/vocabulary/blob/master/vocabulary.md

Endpoint
********

The only endpoint of this version of the api is : https://api.navitia.io/v1/

See `authentication`_ section to find out **how to use your token**.

If you use a web browser, you only have to paste it in the user area, with no password.
Or, in a simplier way, you can add your token in the address bar like :
https://*my-token-is-mine-and-i-will-never-clearly-give-it*@api.navitia.io/v1/coverage/fr-idf/networks


Some easy examples
******************

* Geographical coverage of the service > https://api.navitia.io/v1/coverage 
* Where am I? (WGS 84 coordinates)

    * https://api.navitia.io/v1/coord/2.377310;48.847002
    * I'm on the "/fr-idf" coverage, at "20, rue Hector Malot in Paris, France"

* Which services are available on this coverage? Let's take a look at the links at the bottom of this stream

    * https://api.navitia.io/v1/coverage/fr-idf

* Networks available? (see what network_ is)

    * https://api.navitia.io/v1/coverage/fr-idf/networks
    * pwooo, many networks on this coverage ;)

* Is there any Metro lines or networks? 

    * there is an api for that. See `pt_objects`_ 
    * https://api.navitia.io/v1/coverage/fr-idf/pt_objects?q=metro
    * Response contain one network, one mode, and many lines

* Let's try some filtering (see `ptreferential`_)

    * filter on the specific metro network ("id": "network:OIF:439" extracted from last request)
    * https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/
    * physical modes managed by this network
    * https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/physical_modes
    * metro lines 
    * https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/physical_modes/physical_mode:Metro/lines

* By the way, what stuff are close to me?

    * https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/places_nearby
    * or https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/lines
    * or https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/stop_schedules
    * or ...


Resources
*********

All the resources return a response containing a links object, a paging object, and the requested object.

* **Coverage** : List of the region covered by navitia

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage                                             | List of the areas covered by navitia|
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*                                 | Information about a specific region |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*/coords/*lon;lat*                | Information about a specific region |
+---------------------------------------------------------------+-------------------------------------+

* **Public transportation objects** : List of the public transport objects of a region

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*/*collection_name*               | Collection of objects of a region   |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*/*collection_name*/*object_id*   | Information about a specific region |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*lon;lat*/*collection_name*                 | Collection of objects of a region   |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*lon;lat*/*collection_name*/*object_id*     | Information about a specific region |
+---------------------------------------------------------------+-------------------------------------+

* **Journeys** : Compute journeys

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/journeys                    | List of journeys                    |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /journeys                                             | List of journeys                    |
+---------------------------------------------------------------+-------------------------------------+

* **Route Schedules** : Compute route schedules for a given resource

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/route_schedules             | List of the route schedules         |
+---------------------------------------------------------------+-------------------------------------+

* **Stop Schedules** : Compute stop schedules for a given resource

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/stop_schedules              | List of the stop schedules          |
+---------------------------------------------------------------+-------------------------------------+

* **Departures** : List of the next departures for a given resource

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/departures                  | List of the departures              |
+---------------------------------------------------------------+-------------------------------------+

* **Arrivals** : List of the next departures for a given resource

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/arrivals                    | List of the arrivals                |
+---------------------------------------------------------------+-------------------------------------+

* **Places** : Search in the datas

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/places                                      | List of objects                     |
+---------------------------------------------------------------+-------------------------------------+

* **Places nearby** : List of objects near an object or a coord

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/places_nearby               | List of objects near the resource   |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*lon;lat*/places_nearby                     | List of objects near the resource   |
+---------------------------------------------------------------+-------------------------------------+

.. _authentication:

Authentication
================

You must authenticate to use **navitia.io**. When you register we give you a authentication key to the API.

You must use the `Basic HTTP authentication`_, where the username is the key, and without password.

.. _Basic HTTP authentication: http://tools.ietf.org/html/rfc2617#section-2

.. _paging:

Paging
======

All response contains a paging object

=============== ==== =======================================
Key             Type Description
=============== ==== =======================================
items_per_page  int  Number of items per page
items_on_page   int  Number of items on this page
start_page      int  The page number
total_result    int  Total number of items for this request
=============== ==== =======================================

You can navigate through a request with 2 parameters

=============== ==== =======================================
Parameter       Type Description
=============== ==== =======================================
start_page      int  The page number
count           int  Number of items per page
=============== ==== =======================================

.. _interface:

Interface
=========
We aim to implement `HATEOAS <http://en.wikipedia.org/wiki/HATEOAS>`_ concept with Navitia.

Each response contains a linkable object and lots of links. 
Links allow you to know all accessible uris and services for a given point.

Templated url
*************

Under some link sections, you will find a "templated" property. If "templated" is true, 
then you will have to format the link with one id. 

For example, in response of https://api.navitia.io/v1/coverage/fr-idf/lines 
you will find a *links* section:

.. code-block:: json

	{
		"href": "https://api.navitia.io/v1/coverage/fr-idf/lines/{lines.id}/stop_schedules",
		"rel": "route_schedules",
		"templated": true
	}

You have to put one line id instead of "{lines.id}". For example:
https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:043043001:N1OIF658/stop_schedules

.. _inner-reference:

Inner references
****************

Some link sections look like
	
.. code-block:: json

	{
		"internal": true,
		"type": "disruption",
		"id": "edc46f3a-ad3d-11e4-a5e1-005056a44da2",
		"rel": "disruptions",
		"templated": false
	}

That means you will find inside the same stream ( *"internal": true* ) a "disruptions" section 
( *"rel": "disruptions"* ) containing some disruptions objects ( *"type": "disruption"* ) 
where you can find the details of your object ( *"id": "edc46f3a-ad3d-11e4-a5e1-005056a44da2"* ).

Errors
======

When there's an error you'll receive a response with a error object containing an id

Example
*******

.. code-block:: json

    {
        "error": {
            "id": "bad_filter",
            "message": "ptref : Filters: Unable to find object"
        }
    }

Code 40x
********

This errors appears when there is an error in the request

The are two possible 40x http codes :

* Code 404:

========================== ==========================================================================
Error id                   Description
========================== ==========================================================================
date_out_of_bounds         When the given date is out of bounds of the production dates of the region
no_origin                  Couldn't find an origin for the journeys
no_destination             Couldn't find a destination for the journeys
no_origin_nor_destination  Couldn't find an origin nor a destination for the journeys
no_solution                Couldn't find a journey for known origin and destination
unknown_object             As it's said
========================== ==========================================================================

* Code 400:

=============== ========================================
Error id        Description
=============== ========================================
bad_filter      When you use a custom filter
unable_to_parse When you use a mal-formed custom filter
=============== ========================================

Code 50x
********

Ouch. Technical issue :/

Apis
====

Coverage (/coverage)
********************
You can easily navigate through regions covered by navitia.io, with the coverage api.
The only arguments are the ones of `paging`_.

Inverted geocoding (/coord)
***************************
Very simple service: you give Navitia some coordinates, it answers you where you are:

* your detailed postal address
* the right Navitia "coverage" which allows you to access to all known local mobility services

For example, you can request navitia with a WGS84 coordinate as:

https://api.navitia.io/v1/coord/2.37691590563854;48.8467597481174

In response, you will get the coverage id, a very useful label and a ressource id:

.. code-block:: json

    {
        "regions": [
            "fr-idf"
        ],
        address": {
        ...,
        "label": "20 Rue Hector Malot (Paris)",
        ...,
        "id": "2.37691590564;48.8467597481"
    },

The coverage id is *"regions": ["fr-idf"]* so you can ask Navitia on accessible local mobility services:

https://api.navitia.io/v1/coverage/fr-idf/
	
.. code-block:: json

    {
        "regions": [{
                "status": "running",
                "start_production_date": "20150804","end_production_date": "20160316",
                "id": "fr-idf"
                }],
        "links": [
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/coords"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/places"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/networks"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/physical_modes"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/companies"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/commercial_modes"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/lines"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/routes"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/stop_areas"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/stop_points"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/line_groups"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/connections"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/vehicle_journeys"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/poi_types"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/pois"},
            {"href": "http://api.navitia.io/v1/coverage/fr-idf/"}
            ]
    }

.. _ptreferential:

Public transportation objects exploration (/networks or /lines or /routes...)
*****************************************************************************

Once you have selected a region, you can explore the public transportation objects 
easily with these apis. You just need to add at the end of your url 
a collection name to see all the objects of a particular collection.
To see an object add the id of this object at the end of the collection's url.
The `paging`_ arguments may be used to paginate results.

Collections
###########

* networks
* lines
* routes
* stop_points
* stop_areas
* commercial_modes
* physical_modes
* companies
* vehicle_journeys
* disruptions

Specific parameters
###################

There are other specific parameters.

"depth"
_______

This tiny parameter can expand Navitia power by making it more wordy.
Here is some examples around "metro line 1" from the Parisian network:

* Get "line 1" id 

	* https://api.navitia.io/v1/coverage/fr-idf/pt_objects?q=metro%201
	* The id is "line:OIF:100110001:1OIF439"

* Get routes for this line 

	* https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:100110001:1OIF439/routes

* Want to get a tiny response? Just add "depth=0"

	* https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:100110001:1OIF439/routes?depth=0
	* The response is lighter (parent lines disappear for example)

* Want more informations, just add "depth=2"

	* https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:100110001:1OIF439/routes?depth=2
	* The response is a little more verbose (with some geojson appear in response)

* Wanna fat more informations, let's try "depth=3"

	* https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:100110001:1OIF439/routes?depth=3
	* Big response: all stop_points are shown

* Wanna spam the internet bandwidth? Try "depth=42"

	* No. There is a technical limit with "depth=3"


"odt_level"
___________

- Type: `String`
- Default value: `all`
- Warning: works ONLY with /lines collection...

It allows you to request navitia for specific pickup lines. It refers to the odt_ section.
"odt_level" can take one of these values:

*NEW! after 1.18 versions, this parameter is more accurate*

* all (default value): no filter, provide all public transport lines, whatever its type
* scheduled : provide only regular lines (see the odt_ section)
* with_stops : to get regular, "odt_with_stop_time" and "odt_with_stop_point" lines.

    * You can easily request route_schedule and stop_schedule with these kind of lines.
    * Be aware of "estimated" stop times

* zonal : to get "odt_with_zone" lines with non-detailed trips 

For example

https://api.navitia.io/v1/coverage/fr-nw/networks/network:lila/lines

https://api.navitia.io/v1/coverage/fr-nw/networks/network:Lignes18/lines?odt_level=scheduled

"distance"
__________

- Type: `Integer`
- Default value: 200

If you specify coords in your filter, you can modify the radius used for the proximity search.
https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/stop_schedules?distance=500

"headsign"
__________

- Type: `String`

If given, add a filter on the vehicle journeys that has the given
value as headsign (on vehicle journey itself or at a stop time).

Examples:

* http://api.navitia.io/v1/coverage/fr-idf/vehicle_journeys?headsign=CANE
* http://api.navitia.io/v1/coverage/fr-idf/stop_areas?headsign=CANE

Warning: this last request gives the stop areas used by the vehicle
journeys containing the headsign `CANE`, *not* the stop areas where it
exists a stop time with the headsign `CANE`.

"since" / "until"
_________________

- Type: `iso-date-time`_

To be used only on "vehicle_journeys" collection, to filter on a period.
Both parameters "until" and "since" are optional.

Example:

* https://api.navitia.io/v1/coverage/fr_idf/vehicle_journeys?since=20150912T120000&until=20150913T110000

Warning: this filter is applied using only the first stop time of a vehicle journey,
"since" is included and "until" is excluded.

Filter
######

It is possible to apply a filter to the returned collection, using "filter" parameter.
If no object matches the filter, a "bad_filter" error is sent.
If filter can not be parsed, an "unable_to_parse" error is sent.
If object or attribute provided is not handled, the filter is ignored.

line.code
_________

It allows you to request navitia objects referencing a line whose code is the one provided,
especially lines themselves and routes.

Examples :

* https://api.navitia.io/v1/coverage/fr-idf/lines?filter=line.code=4
* https://api.navitia.io/v1/coverage/fr-idf/routes?filter=line.code="métro 347"


Examples
########

Response example for this request https://api.navitia.io/v1/coverage/fr-idf/physical_modes

.. code-block:: json

    {
        "links": [
            ...
        ],
        "pagination": {
            ...
        },
        "physical_modes": [
            {
                "id": "physical_mode:0x3",
                "name": "Bus"
            },
            {
                "id": "physical_mode:0x4",
                "name": "Ferry"
            },
            ...
        ]
    }

Other examples

* Network list

    * https://api.navitia.io/v1/coverage/fr-idf/networks

* Physical mode list

    * https://api.navitia.io/v1/coverage/fr-idf/physical_modes

* Line list

    * https://api.navitia.io/v1/coverage/fr-idf/lines

* Line list for one mode

    * https://api.navitia.io/v1/coverage/fr-idf/physical_modes/physical_mode:Metro/lines

.. _pt_objects:

Public Transport objects autocomplete (/pt_objects)
***************************************************

This api search in public transport objects via their names. It's a kind of magical autocomplete on public transport data.
It returns, in addition of classic objects, a collection of places_ .

How does it works
#################

Differents kind of objects can be returned (sorted as):

* network
* commercial_mode
* line
* route
* stop_area

Here is a typical use case. A traveler has to find a line between the 1500 lines around Paris. 
Without any filters:

* traveler input: "bob"

    * network : "bobby network"
    * line : "bobby bus 1"
    * line : "bobby bus 2"
    * route : "bobby bus 1 to green"
    * route : "bobby bus 1 to rose"
    * route : "bobby bus 2 to yellow"
    * stop_area : "...

* traveler input: "bobby met"

    * line : "bobby metro 1"
    * line : "bobby metro 11"
    * line : "bobby metro 2"
    * line : "bobby metro 3"
    * route : "bobby metro 1 to Martin"
    * route : "bobby metro 1 to Mahatma"
    * route : "bobby metro 11 to Marcus"
    * route : "bobby metro 11 to Steven"
    * route : "...

* traveler input: "bobby met 11" or "bobby metro 11"

    * line : "bobby metro 11"
    * route : "bobby metro 11 to Marcus"
    * route : "bobby metro 11 to Steven"

Parameters
##########

+---------+---------------+----------------------------+------------------------------------+-----------------------------+
| Required| Name          | Type                       | Description                        | Default value               |
+=========+===============+============================+====================================+=============================+
| yep     | q             | string                     | The search term                    |                             |
+---------+---------------+----------------------------+------------------------------------+-----------------------------+
| nop     | type\[\]      | array of string            | Type of objects you want to query  | \[``network``,              |
|         |               |                            | It takes one the following values: | ``commercial_mode``,        |
|         |               |                            | \[``network``,                     | ``line``,                   |
|         |               |                            | ``commercial_mode``,               | ``route``,                  |
|         |               |                            | ``line``,                          | ``stop_area``\]             |
|         |               |                            | ``route``,                         |                             |
|         |               |                            | ``stop_area``\]                    |                             |
+---------+---------------+----------------------------+------------------------------------+-----------------------------+


+------------------------------------------+
| *Warning*                                |
|                                          |
|    There is no pagination for this api   |
+------------------------------------------+

Example
#######

Response example for : https://api.navitia.io/v1/coverage/fr-idf/pt_objects?q=bus%20ratp%20%39&type[]=line&type[]=route

.. code-block:: json

    {
    "pt_objects": [
        {
            {

                "embedded_type": "line",
                "line": {
                    ...
                },
                "id": "line:OIF:100100091:91OIF442",
                "name": "RATP Bus 91 (MOBILIEN 91)"

            },
                    },
    "links" : [
        ...
     ],
    }


Places autocomplete (/places)
*****************************

This api search in all geographical objects via their names.
It returns, in addition of classic objects, a collection of places_ .
This api is very useful to make some autocomplete stuff.

Differents kind of objects can be returned (sorted as):

* administrative_region
* stop_area
* poi
* address
* stop_point (appears only if specified, using "&type[]=stop_point" filter)

+------------------------------------------+
| *Warning*                                |
|                                          |
|    There is no pagination for this api   |
+------------------------------------------+

Parameters
##########

+---------+---------------+------------------+------------------------------------+-----------------------------+
| Required| Name          | Type             | Description                        | Default value               |
+=========+===============+==================+====================================+=============================+
| yep     | q             | string           | The search term                    |                             |
+---------+---------------+------------------+------------------------------------+-----------------------------+
| nop     | type\[\]      | array of string  | Type of objects you want to query  | \[``stop_area``,            |
|         |               |                  | It takes one the following values: | ``address``,                |
|         |               |                  | \[``stop_area``,                   | ``poi``,                    |
|         |               |                  | ``address``,                       | ``administrative_region``\] |
|         |               |                  | ``administrative_region``,         |                             |
|         |               |                  | ``poi``,                           |                             |
|         |               |                  | ``stop_point``\]                   |                             |
+---------+---------------+------------------+------------------------------------+-----------------------------+
| nop     | admin_uri\[\] | array of string  | If filled, will restrained the     |                             |
|         |               |                  | search within the given admin uris |                             |
+---------+---------------+------------------+------------------------------------+-----------------------------+

How does it works
#################

Example
#######

Response example for : https://api.navitia.io/v1/coverage/fr-idf/places?q=rue

.. code-block:: json

    {
    "places": [
        {
            {

                "embedded_type": "stop_area",
                "stop_area": {
                    ...
                },
                "id": "stop_area:TAN:SA:RUET",
                "name": "Ruette"

            },
                    },
    "links" : [
        ...
     ],
    }

Places Nearby (/places_nearby)
******************************

This api search for public transport object near another object, or near coordinates.
It returns, in addition of classic objects, a collection of places_.

+------------------------------------------+
| *Warning*                                |
|                                          |
|    There is no pagination for this api   |
+------------------------------------------+

Parameters
##########

+---------+---------------+-----------------+------------------------------------------+--------------------------------------+
| Required| name          | Type            | Description                              | Default value                        |
+=========+===============+=================+==========================================+======================================+
| nop     | distance      | int             | Distance range in meters                 | 500                                  |
+---------+---------------+-----------------+------------------------------------------+--------------------------------------+
| nop     | type\[\]      | array of string | Type of objects you want to query        | \[``stop_area``, ``stop_point``,     |
|         |               |                 |                                          | ``poi``, ``administrative_region``\] |
+---------+---------------+-----------------+------------------------------------------+--------------------------------------+
| nop     | admin_uri\[\] | array of string | If filled, will restrained the search    | ""                                   |
|         |               |                 | within the given admin uris              |                                      |
+---------+---------------+-----------------+------------------------------------------+--------------------------------------+
| nop     | filter        | string          | Use to restrain returned objects.        |                                      |
|         |               |                 | for example: places_type.id=theater      |                                      |
+---------+---------------+-----------------+------------------------------------------+--------------------------------------+

Filters can be added:

* request for the city of "Paris" on fr-idf

    * http://api.navitia.io/v1/coverage/fr-idf/places?q=paris

* then pois nearby this city

    * http://api.navitia.io/v1/coverage/fr-idf/places/admin:7444/places_nearby

* and then, let's catch every parking around

    * "distance=10000" Paris is not so big
    * "type[]=poi" to take pois only
    * "filter=poi_type.id=poi_type:amenity:parking" to get parking
    * http://api.navitia.io/v1/coverage/fr-idf/places/admin:7444/places_nearby?distance=10000&count=100&type[]=poi&filter=poi_type.id=poi_type:amenity:parking

Example
########

Response example for this request 
https://api.navitia.io/v1/coverage/fr-idf/stop_areas/stop_area:TRN:SA:DUA8754575/places_nearby

.. code-block:: json

    {
    "places_nearby": [
        {
            "embedded_type": "stop_area",
            "stop_area": {
                "comment": "",
                "name": "CHATEAUDUN",
                "coord": {
                    "lat": "48.073402",
                    "lon": "1.338426"
                },
                "id": "stop_area:TRN:SA:DUA8754575"
            },
            "distance": "0.0",
            "quality": 0,
            "id": "stop_area:TRN:SA:DUA8754575",
            "name": "CHATEAUDUN"
        },
        ....
    }


Journeys (/journeys)
********************

This api computes journeys.

If used within the coverage api, it will retrieve the next journeys from 
the selected public transport object or coordinates.

There are two ways to access this api.

The first one is: `<https://api.navitia.io/v1/{a_path_to_resource}/journeys>`_ it will retrieve 
all the journeys from the resource (*isochrones*).

The other one, the most used, is to access the 'journey' api endpoint: `<https://api.navitia.io/v1/journeys?from={resource_id_1}&to={resource_id_2}&datetime={date_time_to_leave}>`_ .

+-------------------------------------------------------------------------------------------------------------------------------------------------------------+
| *Note*                                                                                                                                                      |
|                                                                                                                                                             |
| Navitia.io handle lot's of different data sets (regions). Some of them can overlap. For example opendata data sets can overlap with private data sets.      |
|                                                                                                                                                             |
| When using the journeys endpoint the data set used to compute the journey is chosen using the possible datasets of the origin and the destination.          |
|                                                                                                                                                             |
| For the moment it is not yet possible to compute journeys on different data sets, but it will one day be possible (with a cross-data-set system).           |
|                                                                                                                                                             |
| If you want to use a specific data set, use the journey api within the data set: `<https://api.navitia.io/v1/coverage/{your_dataset}/journeys>`_            |
+-------------------------------------------------------------------------------------------------------------------------------------------------------------+


+-------------------------------------------------------------------------------------------------------------------------------------------------------------+
| *Note*                                                                                                                                                      |
|                                                                                                                                                             |
| Neither the 'from' nor the 'to' parameter of the journey are required, but obviously one of them has to be provided.                                        |
|                                                                                                                                                             |
| If only one is defined an isochrone is computed with every possible journeys from or to the point.                                                          |
+-------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. _journeys_parameters:

Main parameters
###############

+----------+-----------------------+------------------+-------------------------------------------+-----------------+
| Required | Name                  | Type             | Description                               | Default value   |
+==========+=======================+==================+===========================================+=================+
| nop      | from                  | id               | The id of the departure of your journey   |                 |
|          |                       |                  | If none are provided an isochrone is      |                 |
|          |                       |                  | computed                                  |                 |
+----------+-----------------------+------------------+-------------------------------------------+-----------------+
| nop      | to                    | id               | The id of the arrival of your journey     |                 |
|          |                       |                  | If none are provided an isochrone is      |                 |
|          |                       |                  | computed                                  |                 |
+----------+-----------------------+------------------+-------------------------------------------+-----------------+
| yep      | datetime              | `iso-date-time`_ | Date and time to go                       |                 |
+----------+-----------------------+------------------+-------------------------------------------+-----------------+
| nop      | datetime_represents   | string           | Can be ``departure`` or ``arrival``.      | departure       |
|          |                       |                  |                                           |                 |
|          |                       |                  | If ``departure``, the request will        |                 |
|          |                       |                  | retrieve journeys starting after          |                 |
|          |                       |                  | ``datetime``.                             |                 |
|          |                       |                  |                                           |                 |
|          |                       |                  | If ``arrival`` it will retrieve journeys  |                 |
|          |                       |                  | arriving before ``datetime``.             |                 |
+----------+-----------------------+------------------+-------------------------------------------+-----------------+
| nop      | traveler_type         | enum             | Define speeds and accessibility values    | standard        |
|          |                       |                  | for different kind of people              |                 |
|          |                       |                  |                                           |                 |
|          |                       |                  | * standard                                |                 |
|          |                       |                  | * slow_walker                             |                 |
|          |                       |                  | * fast_walker                             |                 |
|          |                       |                  | * luggage                                 |                 |
|          |                       |                  | * wheelchair                              |                 |
|          |                       |                  |                                           |                 |
|          |                       |                  | Each profile also automatically           |                 |
|          |                       |                  | determines appropriate first and last     |                 |
|          |                       |                  | section modes to the covered area.        |                 |
|          |                       |                  | You can overload all parameters           |                 |
|          |                       |                  | (especially speeds, distances, first and  |                 |
|          |                       |                  | last modes) by setting all of them        |                 |
|          |                       |                  | specifically                              |                 |
+----------+-----------------------+------------------+-------------------------------------------+-----------------+
| nop      | forbidden_uris[]      | id               | If you want to avoid lines, modes,        |                 |
|          |                       |                  | networks, etc.                            |                 |
+----------+-----------------------+------------------+-------------------------------------------+-----------------+
| nop      | data_freshness        | enum             | Define the freshness of data to use to    | base_schedule   |
|          |                       |                  | compute journeys                          |                 |
|          |                       |                  |                                           |                 |
|          |                       |                  | * real_time                               |                 |
|          |                       |                  | * base_schedule                           |                 |
|          |                       |                  |                                           |                 |
|          |                       |                  | when using the following parameter        |                 |
|          |                       |                  |      "&data_freshness=base_schedule"      |                 |
|          |                       |                  | you can get disrupted journeys in the     |                 |
|          |                       |                  | response.                                 |                 |
|          |                       |                  | You can then display the disruption       |                 |
|          |                       |                  | message to the traveler and make a        |                 |
|          |                       |                  | real_time request to get a new            |                 |
|          |                       |                  | undisrupted solution.                     |                 |
|          |                       |                  |                                           |                 |
+----------+-----------------------+------------------+-------------------------------------------+-----------------+


Other parameters
################

+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| Required | Name                  | Type      | Description                               | Default value   |
+==========+=======================+===========+===========================================+=================+
| nop      | first_section_mode[]  | array of  | Force the first section mode if the first | walking         |
|          |                       | string    | section is not a public transport one.    |                 |
|          |                       |           | It takes one the following values:        |                 |
|          |                       |           | ``walking``, ``car``, ``bike``, ``bss``   |                 |
|          |                       |           |                                           |                 |
|          |                       |           | bss stands for bike sharing system        |                 |
|          |                       |           |                                           |                 |
|          |                       |           | It's an array, you can give multiple      |                 |
|          |                       |           | modes                                     |                 |
|          |                       |           |                                           |                 |
|          |                       |           | Note: choosing ``bss`` implicitly allows  |                 |
|          |                       |           | the ``walking`` mode since you might have |                 |
|          |                       |           | to walk to the bss station                |                 |
|          |                       |           |                                           |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | last_section_mode[]   | array of  | Same as first_section_mode but for the    | walking         |
|          |                       | string    | last section                              |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | max_duration_to_pt    | int       | Maximum allowed duration to reach the     | 15*60 s         |
|          |                       |           | public transport                          |                 |
|          |                       |           |                                           |                 |
|          |                       |           | Use this to limit the walking/biking part |                 |
|          |                       |           |                                           |                 |
|          |                       |           | Unit is seconds                           |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | walking_speed         | float     | Walking speed for the fallback sections   | 1.12 m/s        |
|          |                       |           |                                           |                 |
|          |                       |           | Speed unit must be in meter/seconds       | (4 km/h)        |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | bike_speed            | float     | Biking speed for the fallback sections    | 4.1 m/s         |
|          |                       |           |                                           |                 |
|          |                       |           | Speed unit must be in meter/seconds       | (14.7 km/h)     |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | bss_speed             | float     | Speed while using a bike from a bike      | 4.1 m/s         |
|          |                       |           | sharing system for the fallback sections  | (14.7 km/h)     |
|          |                       |           |                                           |                 |
|          |                       |           | Speed unit must be in meter/seconds       |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | car_speed             | float     | Driving speed for the fallback sections   | 16.8 m/s        |
|          |                       |           |                                           |                 |
|          |                       |           | Speed unit must be in meter/seconds       | (60 km/h)       |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | min_nb_journeys       | int       | Minimum number of different suggested     |                 |
|          |                       |           | trips                                     |                 |
|          |                       |           |                                           |                 |
|          |                       |           | More in `multiple_journeys`_              |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | max_nb_journeys       | int       | Maximum number of different suggested     |                 |
|          |                       |           | trips                                     |                 |
|          |                       |           |                                           |                 |
|          |                       |           | More in `multiple_journeys`_              |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | count                 | int       | Fixed number of different journeys        |                 |
|          |                       |           |                                           |                 |
|          |                       |           | More in `multiple_journeys`_              |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | max_nb_tranfers       | int       | Maximum of number transfers               | 10              |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | disruption_active     | boolean   | For compatibility use only.               | False           |
|          |                       |           | If true the algorithm take the disruptions|                 |
|          |                       |           | into account, and thus avoid disrupted    |                 |
|          |                       |           | public transport.                         |                 |
|          |                       |           | Rq: "disruption_active=true" =            |                 |
|          |                       |           |     "data_freshness=real_time"            |                 |
|          |                       |           | Use "data_freshness" parameter instead    |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | wheelchair            | boolean   | If true the traveler is considered to     | False           |
|          |                       |           | be using a wheelchair, thus only          |                 |
|          |                       |           | accessible public transport are used      |                 |
|          |                       |           |                                           |                 |
|          |                       |           | be warned: many data are currently too    |                 |
|          |                       |           | faint to provide acceptable answers       |                 |
|          |                       |           | with this parameter on                    |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | show_codes            | boolean   | If true add internal id in the response   | False           |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | debug                 | boolean   | Debug mode                                | False           |
|          |                       |           |                                           |                 |
|          |                       |           | No journeys are filtered in this mode     |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+

Objects
#######

Here is a typical journey, all sections are detailed below

.. image:: typical_itinerary.png


Main response object
____________________

=================== ==================== ===========================================================================
Field               Type                 Description
=================== ==================== ===========================================================================
journeys            array of `journey`_  List of computed journeys
links               link_                Links related to the journeys
=================== ==================== ===========================================================================

.. _journey:

Journey object
______________

+---------------------+--------------------------+--------------------------------------------------------------+
| Field               | Type                     | Description                                                  |
+=====================+==========================+==============================================================+
| duration            | int                      | Duration of the journey                                      |
+---------------------+--------------------------+--------------------------------------------------------------+
| nb_transfers        | int                      | Number of transfers in the journey                           |
+---------------------+--------------------------+--------------------------------------------------------------+
| departure_date_time | `iso-date-time`_         | Departure date and time of the journey                       |
+---------------------+--------------------------+--------------------------------------------------------------+
| requested_date_time | `iso-date-time`_         | Requested date and time of the journey                       |
+---------------------+--------------------------+--------------------------------------------------------------+
| arrival_date_time   | `iso-date-time`_         | Arrival date and time of the journey                         |
+---------------------+--------------------------+--------------------------------------------------------------+
| sections            | array of `section`_      | All the sections of the journey                              |
+---------------------+--------------------------+--------------------------------------------------------------+
| from                | places_                  | The place from where the journey starts                      |
+---------------------+--------------------------+--------------------------------------------------------------+
| to                  | places_                  | The place from where the journey ends                        |
+---------------------+--------------------------+--------------------------------------------------------------+
| links               | `link`_                  | Links related to this journey                                |
+---------------------+--------------------------+--------------------------------------------------------------+
| type                | *enum* string            | Used to qualify a journey.                                   |
|                     |                          | See the `journey-qualif`_ section for more information       |
+---------------------+--------------------------+--------------------------------------------------------------+
| fare                | fare_                    | Fare of the journey (tickets and price)                      |
+---------------------+--------------------------+--------------------------------------------------------------+
| tags                | array of string          | List of tags on the journey. The tags add additional         |
|                     |                          | information on the journey beside the journey type.          |
|                     |                          | See for example `multiple_journeys`_.                        |
+---------------------+--------------------------+--------------------------------------------------------------+
| status              | *enum*                   | Status from the whole journey taking into acount the most    |
|                     |                          | disturbing information retrieved on every object used.       |
|                     |                          | Can be:                                                      |
|                     |                          |                                                              |
|                     |                          | * NO_SERVICE                                                 |
|                     |                          | * REDUCED_SERVICE                                            |
|                     |                          | * SIGNIFICANT_DELAYS                                         |
|                     |                          | * DETOUR                                                     |
|                     |                          | * ADDITIONAL_SERVICE                                         |
|                     |                          | * MODIFIED_SERVICE                                           |
|                     |                          | * OTHER_EFFECT                                               |
|                     |                          | * UNKNOWN_EFFECT                                             |
|                     |                          | * STOP_MOVED                                                 |
|                     |                          |                                                              |
|                     |                          | In order to get a undisrupted journey, you just have to add  |
|                     |                          | a "&data_freshness=real_time" parameter                      |
+---------------------+--------------------------+--------------------------------------------------------------+


+-----------------------------------------------------------------------------------------------------------+
| *Note*                                                                                                    |
|                                                                                                           |
| When used with just a "from" or a "to" parameter, it will not contain any sections                        |
+-----------------------------------------------------------------------------------------------------------+

.. _section:

Section object
______________


+-------------------------+------------------------------------+----------------------------------------------------+
| Field                   | Type                               | Description                                        |
+=========================+====================================+====================================================+
| type                    | *enum* string                      | Type of the section, it can be:                    |
|                         |                                    |                                                    |
|                         |                                    | * ``public_transport``: public transport section   |
|                         |                                    | * ``street_network``: street section               |
|                         |                                    | * ``waiting``: waiting section between transport   |
|                         |                                    | * ``stay_in``: this "stay in the vehicle" section  |
|                         |                                    |   occurs when the traveller has to stay in the     |
|                         |                                    |   vehicle when the bus change its routing.         |
|                         |                                    |   Here is an exemple for a journey from A to B:    |
|                         |                                    |   (lollipop line)                                  |
|                         |                                    |                                                    |
|                         |                                    |   .. image:: stay_in.png                           |
|                         |                                    |                                                    |
|                         |                                    | * ``transfer``: transfert section                  | 
|                         |                                    | * ``crow_fly``: teleportation section.             |
|                         |                                    |   Used when starting or arriving to a city or a    |
|                         |                                    |   stoparea ("potato shaped" objects)               |
|                         |                                    |   Useful to make navitia idempotent.               |
|                         |                                    |   Be careful: no "path" nor "geojson" items in     |
|                         |                                    |   this case                                        |
|                         |                                    |                                                    |
|                         |                                    |   .. image:: crow_fly.png                          |
|                         |                                    |      :scale: 25 %                                  |
|                         |                                    |                                                    |
|                         |                                    | * ``on_demand_transport``: vehicle may not drive   |
|                         |                                    |   along: traveler will have to call agency to      |
|                         |                                    |   confirm journey                                  |
|                         |                                    | * ``bss_rent``: taking a bike from a bike sharing  |
|                         |                                    |   system (bss)                                     |
|                         |                                    | * ``bss_put_back``: putting back a bike from a bike|
|                         |                                    |   sharing system (bss)                             |
|                         |                                    | * ``boarding``: boarding on plane                  |
|                         |                                    | * ``landing``: landing off the plane               |
+-------------------------+------------------------------------+----------------------------------------------------+
| id                      | string                             | Id of the section                                  |      
+-------------------------+------------------------------------+----------------------------------------------------+
| mode                    | *enum* string                      | Mode of the street network:                        |      
|                         |                                    |   ``Walking``, ``Bike``, ``Car``                   |        
+-------------------------+------------------------------------+----------------------------------------------------+
| duration                | int                                | Duration of this section                           |      
+-------------------------+------------------------------------+----------------------------------------------------+
| from                    | places_                            | Origin place of this section                       |      
+-------------------------+------------------------------------+----------------------------------------------------+
| to                      | places_                            | Destination place of this section                  |      
+-------------------------+------------------------------------+----------------------------------------------------+
| links                   | Array of link_                     | Links related to this section                      |      
+-------------------------+------------------------------------+----------------------------------------------------+
| display_informations    | display_informations_              | Useful information to display                      |      
+-------------------------+------------------------------------+----------------------------------------------------+
| additionnal_informations| *enum* string                      | Other information. It can be:                      |
|                         |                                    |                                                    |
|                         |                                    | * ``regular``: no on demand transport (odt)        |
|                         |                                    | * ``has_date_time_estimated``: section with at     |
|                         |                                    |   least one estimated date time                    |
|                         |                                    | * ``odt_with_stop_time``: odt with                 |
|                         |                                    |   fixed schedule, but travelers have to call       |
|                         |                                    |   agency!                                          |
|                         |                                    | * ``odt_with_stop_point``: odt where pickup or     | 
|                         |                                    |   drop off are specific points                     |
|                         |                                    | * ``odt_with_zone``: odt which is like a cab,      |
|                         |                                    |   from wherever you want to wherever you want,     |
|                         |                                    |   whenever it is possible                          |
+-------------------------+------------------------------------+----------------------------------------------------+
| geojson                 | `GeoJson <http://www.geojson.org>`_|                                                    |        
+-------------------------+------------------------------------+----------------------------------------------------+
| path                    | Array of path_                     | The path of this section                           |
+-------------------------+------------------------------------+----------------------------------------------------+
| transfer_type           | *enum* string                      | The type of this transfer it can be: ``walking``,  |
|                         |                                    |  ``guaranteed``, ``extension``                     |
+-------------------------+------------------------------------+----------------------------------------------------+
| stop_date_times         | Array of stop_date_time_           | List of the stop times of this section             |
+-------------------------+------------------------------------+----------------------------------------------------+
| departure_date_time     | `iso-date-time`_                   | Date and time of departure                         |
+-------------------------+------------------------------------+----------------------------------------------------+
| arrival_date_time       | `iso-date-time`_                   | Date and time of arrival                           |
+-------------------------+------------------------------------+----------------------------------------------------+


.. _path:

Path object
___________

  A path object in composed of an array of path_item_ (segment).

.. _path_item:

Path item object
________________

+--------------------------+--------------------------------------+--------------------------------------------------------+
| Field                    | Type                                 | Description                                            |
+==========================+======================================+========================================================+
| length                   | int                                  | Length (in meter) of the segment                       |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| name                     | string                               | name of the way corresponding to the segment           |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| duration                 | int                                  | duration (in seconds) of the segment                   |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| direction                | int                                  | Angle (in degree) between the previous segment and     |
|                          |                                      | this segment.                                          |
|                          |                                      |                                                        |
|                          |                                      | * 0 means going straight                               |
|                          |                                      |                                                        |
|                          |                                      | * > 0 means turning right                              |
|                          |                                      |                                                        |
|                          |                                      | * < 0 means turning left                               |
|                          |                                      |                                                        |
|                          |                                      | Hope it's easier to understand with a picture:         |
|                          |                                      |                                                        |
|                          |                                      | .. image:: direction.png                               |
|                          |                                      |    :scale: 50 %                                        |
+--------------------------+--------------------------------------+--------------------------------------------------------+

.. _fare:

Fare object
___________


===================== =========================== ===================================================================
Field                 Type                        Description
===================== =========================== ===================================================================
total                 cost_                       total cost of the journey
found                 boolean                     False if no fare has been found for the journey, True otherwise
links                 link_                       Links related to this object. Link with related `tickets <ticket>`_
===================== =========================== ===================================================================

.. _cost:

Cost object
___________

===================== =========================== =============
Field                 Type                        Description
===================== =========================== =============
value                 float                       cost
currency              string                      currency
===================== =========================== =============

.. _ticket:

Ticket object 
_____________

===================== =========================== ========================================
Field                 Type                        Description
===================== =========================== ========================================
id                    string                      Id of the ticket    
name                  string                      Name of the ticket
found                 boolean                     False if unknown ticket, True otherwise
cost                  cost_                       Cost of the ticket
links                 array of link_              Link to the section_ using this ticket
===================== =========================== ========================================


Route Schedules and time tables (/route_schedules)
***************************************************

This api gives you access to schedules of routes.
The response is made of an array of route_schedule, and another one of `note`_.
You can access it via that kind of url: `<https://api.navitia.io/v1/{a_path_to_a_resource}/route_schedules>`_

Parameters
##########

+----------+---------------------+-------------------+------------------------------+---------------+
| Required | Name                | Type              | Description                  | Default Value |
+==========+=====================+===================+==============================+===============+
| yep      | from_datetime       | `iso-date-time`_  | The date_time from           |               |
|          |                     |                   | which you want the schedules |               |
+----------+---------------------+-------------------+------------------------------+---------------+
| nop      | duration            | int               | Maximum duration in seconds  | 86400         |
|          |                     |                   | between from_datetime        |               |
|          |                     |                   | and the retrieved datetimes. |               |
+----------+---------------------+-------------------+------------------------------+---------------+
| nop      | max_date_times      | int               | Maximum number of            |               |
|          |                     |                   | columns per                  |               |
|          |                     |                   | schedule.                    |               |
+----------+---------------------+-------------------+------------------------------+---------------+
| nop      | data_freshness      | enum              | Define the freshness of data | base_schedule |
|          |                     |                   | to use                       |               |
|          |                     |                   |                              |               |
|          |                     |                   | * real_time                  |               |
|          |                     |                   | * base_schedule              |               |
|          |                     |                   |                              |               |
+----------+---------------------+-------------------+------------------------------+---------------+

Objects
#######

route_schedule object
_____________________

===================== =========================== ==============================================
Field                 Type                        Description
===================== =========================== ==============================================
display_informations  `display_informations`_     Usefull information about the route to display
Table                 table_                      The schedule table
===================== =========================== ==============================================

.. _table:

table object
____________

======= ================= ====================================
Field   Type              Description
======= ================= ====================================
Headers Array of header_  Informations about vehicle journeys
Rows    Array of row_     A row of the schedule
======= ================= ====================================

.. _header:

header object
_____________

+--------------------------+-----------------------------+-----------------------------------+
| Field                    | Type                        | Description                       |
+==========================+=============================+===================================+
| additionnal_informations | Array of String             | Other information: TODO enum      |
+--------------------------+-----------------------------+-----------------------------------+
| display_informations     | `display_informations`_     | Usefull information about the     |
|                          |                             | the vehicle journey to display    |
+--------------------------+-----------------------------+-----------------------------------+
| links                    | Array of link_              | Links to line_, vehicle_journey,  |
|                          |                             | route_, commercial_mode_,         |
|                          |                             | physical_mode_, network_          |
+--------------------------+-----------------------------+-----------------------------------+

.. _row:

row object
__________


+------------+----------------------------------------------+---------------------------+
| Field      | Type                                         | Description               |
+============+==============================================+===========================+
| date_times | Array of `pt-date-time`_                     | Array of public transport |
|            |                                              | date time                 |
+------------+----------------------------------------------+---------------------------+
| stop_point | `stop_point`_                                | The stop point of the row |
+------------+----------------------------------------------+---------------------------+



Stop Schedules and other kind of time tables (/stop_schedules)
**************************************************************

This api gives you access to schedules of stops going through a stop point.
The response is made of an array of stop_schedule, and another one of `note`_.
You can access it via that kind of url: `<https://api.navitia.io/v1/{a_path_to_a_resource}/stop_schedules>`_

Parameters
##########

+----------+---------------------+------------------+---------------------------------------+---------------+
| Required | Name                | Type             | Description                           | Default Value |
+==========+=====================+==================+=======================================+===============+
| yep      | from_datetime       | `iso-date-time`_ | The date_time from                    |               |
|          |                     |                  | which you want the schedules          |               |
+----------+---------------------+------------------+---------------------------------------+---------------+
| nop      | duration            | int              | Maximum duration in seconds           | 86400         |
|          |                     |                  | between from_datetime                 |               |
|          |                     |                  | and the retrieved datetimes.          |               |
+----------+---------------------+------------------+---------------------------------------+---------------+
| nop      | data_freshness      | enum             | Define the freshness of data to use   | base_schedule |
|          |                     |                  | to compute journeys                   |               |
|          |                     |                  |                                       |               |
|          |                     |                  | * real_time                           |               |
|          |                     |                  | * base_schedule                       |               |
|          |                     |                  |                                       |               |
+----------+---------------------+------------------+---------------------------------------+---------------+

Stop_schedule object
####################

===================== =============================================== ==============================================
Field                 Type                                            Description
===================== =============================================== ==============================================
display_informations  display_informations_                           Usefull information about the route to display
route                 route_                                          The route of the schedule
date_times            Array of `pt-date-time`_                        When does a bus stops at the stop point
stop_point            stop_point_                                     The stop point of the schedule
===================== =============================================== ==============================================

Departures (/departures)
************************

This api retrieves a list of departures from a datetime of a selected object.
Departures are ordered chronologically in ascending order.

Parameters
##########

+----------+---------------------+------------------+---------------------------------------+---------------+
| Required | Name                | Type             | Description                           | Default Value |
+==========+=====================+==================+=======================================+===============+
| yep      | from_datetime       | `iso-date-time`_ | The date_time from                    |               |
|          |                     |                  | which you want the schedules          |               |
+----------+---------------------+------------------+---------------------------------------+---------------+
| nop      | duration            | int              | Maximum duration in seconds           | 86400         |
|          |                     |                  | between from_datetime                 |               |
|          |                     |                  | and the retrieved datetimes.          |               |
+----------+---------------------+------------------+---------------------------------------+---------------+
| nop      | data_freshness      | enum             | Define the freshness of data to use   | real_time     |
|          |                     |                  | to compute journeys                   |               |
|          |                     |                  |                                       |               |
|          |                     |                  | * real_time                           |               |
|          |                     |                  | * base_schedule                       |               |
|          |                     |                  |                                       |               |
+----------+---------------------+------------------+---------------------------------------+---------------+

Departure objects
#################

===================== =========================== ========================================
Field                 Type                        Description
===================== =========================== ========================================
route                 route_                      The route of the schedule
stop_date_time        Array of `stop_date_time`_  When does a bus stops at the stop point
stop_point            stop_point_                 The stop point of the schedule
===================== =========================== ========================================

Arrivals (/arrivals)
********************

This api retrieves a list of arrivals from a datetime of a selected object.
Arrivals are ordered chronologically in ascending order.

Traffic reports (/traffic_reports)
**********************************

This service provides the state of public transport traffic.
It can be called for an overall coverage or for a specific object. 

Parameters
##########

You can access it via that kind of url: `<https://api.navitia.io/v1/{a_path_to_a_resource}/traffic_reports>`_

For example:

* overall public transport traffic report on Ile de France coverage

    * https://api.navitia.io/v1/coverage/fr-idf/traffic_reports

* Is there any perturbations on the RER network ?

    * https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/traffic_reports

* Is there any perturbations on the "RER A" line ?

    * https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/lines/line:TRN:DUA810801043/traffic_reports

The response is made of an array of `traffic_reports`_, and another one of `disruptions`_.
There are inner links between this 2 arrays: see the `inner-reference`_ section to use them.

Objects
#######

.. _traffic_reports:

Traffic_reports objects
_______________________


Traffic_reports is an array of some traffic_report object. 
One traffic_report object is a complex object, made of a network, an array of lines and an array of stop_areas.
A typical traffic_report object will contain:

* 1 network which is the grouping object

    * it can contain links to its disruptions. These disruptions are globals and might not be applied on lines or stop_areas.

* 0..n lines 

    * each line contains at least a link to its disruptions

* 0..n stop_areas

    * each stop_area contains at least a link to its disruptions

It means that if a stop_area is used by many networks, it will appear many times.

Here is a typical response

.. code-block:: json

    {
    "traffic_reports": [
        "network": {"name": "bob", "links": [], "id": "network:bob"},
        "lines": [
            {
            "code": "1",
             ... ,
            "links": [ {
                "internal": true,
                "type": "disruption",
                "id": "link-to-green",
                "rel": "disruptions",
                "templated": false
                } ]
            },
            {
            "code": "12",
             ... ,
            "links": [ {
                "internal": true,
                "type": "disruption",
                "id": "link-to-pink",
                "rel": "disruptions",
                "templated": false
                }]
            },
        ],
        "stop_areas": [
            {
            "name": "bobito", 
             ... ,
            "links": [ {
                "internal": true,
                "type": "disruption",
                "id": "link-to-red",
                "rel": "disruptions",
                "templated": false
                }]
            },
        ],
     ],
     [
        "network": {
            "name": "bobette", 
            "id": "network:bobette",
            "links": [ {
                "internal": true,
                "type": "disruption",
                "id": "link-to-blue",
                "rel": "disruptions",
                "templated": false
                }]
            },
        "lines": [
            {
            "code": "A",
             ... ,
            "links": [ {
                "internal": true,
                "type": "disruption",
                "id": "link-to-green",
                "rel": "disruptions",
                "templated": false
                } ]
            },
            {
            "code": "C",
             ... ,
            "links": [ {
                "internal": true,
                "type": "disruption",
                "id": "link-to-yellow",
                "rel": "disruptions",
                "templated": false
                }]
            },
        "stop_areas": [
            {
            "name": "bobito", 
             ... ,
            "links": [ {
                "internal": true,
                "type": "disruption",
                "id": "link-to-red",
                "rel": "disruptions",
                "templated": false
                }]
            },
        ],

    ],
    "disruptions": [ 
        {
            "status": "active", 
            "severity": {"color": "", "priority": 4, "name": "Information", "effect": "UNKNOWN_EFFECT"},
            "messages": [ { "text": "green, super green", ...} ],
            "id": "link-to-green"},
            ...
        },{
            "status": "futur",
            "messages": [ { "text": "pink, floyd pink", ... } ],
            "id": "link-to-pink"},
            ...
        },{
            "status": "futur",
            "messages": [ { "text": "red, mine", ... } ],
            "id": "link-to-red"},
            ...
        },{
            "status": "futur",
            "messages": [ { "text": "blue, grass", ... } ],
            "id": "link-to-blue"},
            ...
        },{
            "status": "futur",
            "messages": [ { "text": "yellow, submarine", ... } 
            "id": "link-to-yellow"},
            ...}
        ],
    "link": { ... },
    "pagination": { ... }
    }

This typical response means:

* traffic_reports

    * network "bob"

        * line "1" > internal link to disruption "green"
        * line "12" > internal link to disruption "pink"
        * stop_area "bobito" > internal link to disruption "red"

    * network "bobette" > internal link to disruption "blue"

        * line "A" > internal link to disruption "green"
        * line "C" > internal link to disruption "yellow"
        * stop_area "bobito" > internal link to disruption "red"

* disruptions (disruption target links)

    * disruption "green"
    * disruption "pink"
    * disruption "red"
    * disruption "blue"
    * disruption "yellow"
    * Each disruption contains the messages to show.

Details for disruption objects : `disruptions`_

Objects
=======

Standard objects
****************

.. _coord:

Coord
#####

Lots of object are geographically localized :

====== ====== ============
Field  Type   Description
====== ====== ============
lon    float  Longitude
lat    float  Latitude
====== ====== ============

.. _iso-date-time:

ISO-date-time
#############

Navitia manages every date or time as a UTC date-time. The web-service 

* exposes every date times as local times via an ISO 8601 "YYYYMMDDThhmmss" string
* can be request using local times via an ISO 8601 "YYYYMMDDThhmmss" string

For example: `<https://api.navitia.io/v1/journeys?from=bob&to=bobette&datetime=20140425T1337>`_

There are lots of ISO 8601 libraries in every kind of language that you should use before breaking down `<https://youtu.be/-5wpm-gesOY>`_

Public transport objects
************************

.. _network:

Network
#######

Networks are fed by agencies in GTFS format.

====== ============= ==========================
Field  Type          Description
====== ============= ==========================
id     string        Identifier of the network
name   string        Name of the network
====== ============= ==========================

.. _line:

Line
#####

=============== ====================== ============================
Field           Type                   Description
=============== ====================== ============================
id              string                 Identifier of the line
name            string                 Name of the line
code            string                 Code name of the line
color           string                 Color of the line
routes          array of `route`_      Routes of the line
commercial_mode `commercial_mode`_     Commercial mode of the line
=============== ====================== ============================

.. _route:

Route
#####

============ ===================== ==================================
Field        Type                  Description
============ ===================== ==================================
id           string                Identifier of the route
name         string                Name of the route
is_frequence bool                  Is the route has frequency or not
line         `line`_               The line of this route
direction    places_               The direction of this route
============ ===================== ==================================

As "direction" is a places_ , it can be a poi in some data.

.. _stop_point:

Stop Point
##########

======================= ===================== =====================================================================
Field                   Type                  Description
======================= ===================== =====================================================================
id                      string                Identifier of the stop point
name                    string                Name of the stop point
coord                   `coord`_              Coordinates of the stop point
administrative_regions  array of `admin`_     Administrative regions of the stop point in which is the stop point
equipments              array of string       list of `equipment`_ of the stop point
stop_area               `stop_area`_          Stop Area containing this stop point
======================= ===================== =====================================================================


.. _stop_area:

Stop Area
#########

====================== =========================== ==================================================================
Field                  Type                        Description
====================== =========================== ==================================================================
id                     string                      Identifier of the stop area
name                   string                      Name of the stop area
coord                  `coord`_                    Coordinates of the stop area
administrative_regions array of `admin`_           Administrative regions of the stop area in which is the stop area
stop_points            array of `stop_point`_      Stop points contained in this stop area
====================== =========================== ==================================================================


.. _commercial_mode:

Commercial Mode
###############

================ =============================== =======================================
Field            Type                            Description
================ =============================== =======================================
id               string                          Identifier of the commercial mode
name             string                          Name of the commercial mode
physical_modes   array of `physical_mode`_       Physical modes of this commercial mode
================ =============================== =======================================


.. _physical_mode:

Physical Mode
#############

==================== ================================ ========================================
Field                Type                             Description
==================== ================================ ========================================
id                   string                           Identifier of the physical mode
name                 string                           Name of the physical mode
commercial_modes     array of `commercial_mode`_      Commercial modes of this physical mode
==================== ================================ ========================================

Physical modes are fastened and normalized. If you want to propose modes filter in your application,
you should use `physical_mode`_ rather than `commercial_mode`_.

Here is the valid id list:

* physical_mode:Air
* physical_mode:Boat
* physical_mode:Bus
* physical_mode:BusRapidTransit
* physical_mode:Coach
* physical_mode:Ferry
* physical_mode:Funicular
* physical_mode:LocalTrain
* physical_mode:LongDistanceTrain
* physical_mode:Metro
* physical_mode:RapidTransit
* physical_mode:Shuttle
* physical_mode:Taxi
* physical_mode:Train
* physical_mode:Tramway

You can use these ids in the forbidden_uris[] parameter from `journeys_parameters`_ for exemple.

.. _company:

Company
#######

==================== ============================= =================================
Field                Type                               Description
==================== ============================= =================================
id                   string                             Identifier of the company
name                 string                             Name of the company
==================== ============================= =================================

.. _places:

Place
#####

A container containing either a `admin`_, `poi`_, `address`_, `stop_area`_, `stop_point`_.

===================== ============================= =================================
Field                 Type                          Description
===================== ============================= =================================
name                  string                        The name of the embedded object
id                    string                        The id of the embedded object
embedded_type         `embedded_type`_              The type of the embedded object
administrative_region *optional* `admin`_           Embedded administrative region
stop_area             *optional* `stop_area`_       Embedded Stop area
poi                   *optional* `poi`_             Embedded poi
address               *optional* `address`_         Embedded address
stop_point            *optional* `stop_point`_      Embedded Stop point
===================== ============================= =================================

.. _pt_object:

pt_object
#########

A container containing either a `network`_, `commercial_mode`_, `line`_, 
`route`_, `stop_area`_, `stop_point`_

===================== ============================= =================================
Field                 Type                          Description
===================== ============================= =================================
name                  string                        The name of the embedded object
id                    string                        The id of the embedded object
embedded_type         `embedded_type`_        The type of the embedded object
stop_area             *optional* `stop_area`_       Embedded Stop area
stop_point            *optional* `stop_point`_      Embedded Stop point
network               *optional* `network`_         Embedded network
commercial_mode       *optional* `commercial_mode`_ Embedded commercial_mode
stop_area             *optional* `stop_area`_       Embedded Stop area
line                  *optional* `line`_            Embedded line
route                 *optional* `route`_           Embedded route
===================== ============================= =================================


Real time and disruption objects
********************************

.. _disruptions:

Disruption
##########

===================== ========================================== ===================================================
Field                 Type                                       Description
===================== ========================================== ===================================================
status                between: "past", "active" or "future"      state of the disruption         
id                    string                                     Id of the disruption
disruption_id         string                                     for traceability: Id of original input disruption
severity              `severity`_                                gives some categorization element
application_periods   array of `period`_                         dates where the current disruption is active
messages              `message`_                                 text to provide to the traveler
updated_at            `iso-date-time`_                           date_time of last modifications 
impacted_objects      array of `pt_object`_                      The list of public transport objects which are 
                                                                 affected by the disruption
cause                 string                                     why is there such a disruption?
===================== ========================================== ===================================================

.. _message:

Messages
########

===================== ==================== ==========================================================
Field                 Type                 Description
===================== ==================== ==========================================================
text                  string               a message to bring to a traveler
channel               `channel`_           destination media. Be careful, no normalized enum for now
===================== ==================== ==========================================================

.. _severity:

Severity
########

Severity object can be used to make visual grouping.

+--------------------+------------------+-----------------------------------------------------------------------+
| Field              | Type             | Description                                                           |
+====================+==================+=======================================================================+
| color              |  string          | HTML color for classification                                         |
+--------------------+------------------+-----------------------------------------------------------------------+
| priority           |  integer         | given by the agency : 0 is strongest priority. it can be null         |
+--------------------+------------------+-----------------------------------------------------------------------+
| name               |  string          | name of severity                                                      |
+--------------------+------------------+-----------------------------------------------------------------------+
| effect             |  Enum            | Normalized value of the effect on the public transport object         |
|                    |                  | See the GTFS RT documentation at                                      |
|                    |                  | https://developers.google.com/transit/gtfs-realtime/reference#Effect  |
+--------------------+------------------+-----------------------------------------------------------------------+

.. _channel:

Channel
#######

+---------------------+------------------+-----------------------------------------------------------------------+
| Field               | Type             | Description                                                           |
+=====================+==================+=======================================================================+
| id                  | string           | Identifier of the address                                             |
+---------------------+------------------+-----------------------------------------------------------------------+
| content_type        | string           | Like text/html, you know? Otherwise, take a look at                   |
|                     |                  | http://www.w3.org/Protocols/rfc1341/4_Content-Type.html               |
+---------------------+------------------+-----------------------------------------------------------------------+
| name                | string           | name of the Channel                                                   |
+---------------------+------------------+-----------------------------------------------------------------------+

.. _period:

Period
######

===================== =============================================== ==============================================
Field                 Type                                            Description
===================== =============================================== ==============================================
begin                 `iso-date-time`_                                Beginning date and time of an activity period
end                   `iso-date-time`_                                Closing date and time of an activity period
===================== =============================================== ==============================================



Street network objects
**********************

.. _poi:

Poi
###

Poi = Point Of Interest

================ ================================== =======================================
Field            Type                               Description
================ ================================== =======================================
id               string                             Identifier of the poi
name             string                             Name of the poi
poi_type         `poi_type`_                        Type of the poi
================ ================================== =======================================

.. _poi_type:

Poi Type
########

================ ================================== =======================================
Field            Type                               Description
================ ================================== =======================================
id               string                             Identifier of the poi type
name             string                             Name of the poi type
================ ================================== =======================================

.. _address:

Address
#######

====================== =========================== ==================================================================
Field                  Type                        Description
====================== =========================== ==================================================================
id                     string                      Identifier of the address
name                   string                      Name of the address
coord                  `coord`_                    Coordinates of the address
house_number           int                         House number of the address
administrative_regions array of `admin`_           Administrative regions of the address in which is the stop area
====================== =========================== ==================================================================

.. _admin:

Administrative region
#####################


===================== =========================== ==================================================================
Field                 Type                        Description
===================== =========================== ==================================================================
id                    string                      Identifier of the address
name                  string                      Name of the address
coord                 `coord`_                    Coordinates of the address
level                 int                         Level of the admin
zip_code              string                      Zip code of the admin
===================== =========================== ==================================================================

Cities are mainly on the 8 level, dependant on the country (http://wiki.openstreetmap.org/wiki/Tag:boundary%3Dadministrative)

Other objects
*************

.. _pt-date-time:

pt-date-time
############

pt-date-time (pt stands for "public transport") is a complex date time object to manage the difference between stop and leaving times at a stop.

+--------------------------+----------------------+--------------------------------+
| Field                    | Type                 | Description                    |
+==========================+======================+================================+
| additionnal_informations | Array of String      | Other information: TODO enum   |
+--------------------------+----------------------+--------------------------------+
| departure_date_time      | `iso-date-time`_     | A date time                    |
+--------------------------+----------------------+--------------------------------+
| arrival_date_time        | `iso-date-time`_     | A date time                    |
+--------------------------+----------------------+--------------------------------+
| links                    | Array of link_       | internal links to notes        |
+--------------------------+----------------------+--------------------------------+

.. _note:

note
####

===== ====== ========================
Field Type   Description
===== ====== ========================
id    String id of the note
value String The content of the note
===== ====== ========================

.. _stop_date_time:

stop_date_time
##############

========== ===================================== ============================
Field      Type                                  Description
========== ===================================== ============================
date_time  `pt-date-time`_                       A public transport date time
stop_point stop_point_                           A stop point
========== ===================================== ============================

.. _embedded_type:

Embedded type
#############

Enum use to identify what kind of objects "/place", "/pt_objects" or "/disruptions" API are managing.

+------------------------------------------------------------------------+
| *Note*                                                                 |
|                                                                        |
|    This enum is used by 3 API:                                         |
|                                                                        |
|    * Using /places API, navitia would returned objects among           |
|    administrative_region, stop_area, poi, address and stop_point types |
|                                                                        |
|    * Using /pt_objects API, navitia would returned objects among       |
|    network, commercial_mode, stop_area, line and route types           |
|                                                                        |
|    * Using /disruptions API, navitia would returned objects among      |
|    network, commercial_mode, stop_area, line, route and trips types    |
|                                                                        |
+------------------------------------------------------------------------+

===================== ============================================================
Value                 Description
===================== ============================================================
administrative_region a city, a district, a neighborhood
network               a public transport network
commercial_mode       a public transport branded mode
line                  a public transport line
route                 a public transport route
stop_area             a nameable zone, where there are some stop points  
stop_point            a location where vehicles can pickup or drop off passengers
address               a point located in a street
poi                   a point of interest
===================== ============================================================

.. _equipment:

equipment
#########

Enum from

* has_wheelchair_accessibility
* has_bike_accepted
* has_air_conditioned
* has_visual_announcement
* has_audible_announcement
* has_appropriate_escort
* has_appropriate_signage
* has_school_vehicle
* has_wheelchair_boarding
* has_sheltered
* has_elevator
* has_escalator
* has_bike_depot


.. _display_informations:

display_informations
####################

=============== =============== ==================================
Field           Type            Description
=============== =============== ==================================
network         String          The name of the network
direction       String          A direction
commercial_mode String          The commercial mode
physical_mode   String          The physical mode
label           String          The label of the object
color           String          The hexadecimal code of the line
code            String          The code of the line
description     String          A description
equipments      Array of string list of `equipment`_ of the object
=============== =============== ==================================

.. _link:

link
####

See `interface`_ section.

Misc mechanisms (and few boring stuff)
======================================

.. _multiple_journeys: 

Multiple journeys
*****************

Navitia can compute several kind of trips within a journey query.

The `RAPTOR <http://research.microsoft.com/apps/pubs/default.aspx?id=156567>`_ algorithm used in Navitia is a multi-objective algorithm. Thus it might return multiple journeys if it cannot know that one is better than the other. 
For example it cannot decide that a one hour trip with no connection is better than a 45 minutes trip with one connection (it is called the `pareto front <http://en.wikipedia.org/wiki/Pareto_efficiency>`_).

If the user asks for more journeys than the number of journeys given by RAPTOR (with the parameter ``min_nb_journeys`` or ``count``), Navitia will ask RAPTOR again, 
but for the following journeys (or the previous ones if the user asked with ``datetime_represents=arrival``). 

Those journeys have the ``next`` (or ``previous``) value in their tags.


.. _journey-qualif:

Journey qualification process
*****************************

Since Navitia can return several journeys, it tags them to help the user choose the best one for his needs.

The different journey types are:

===================== ========================================================== 
Type                  Description
===================== ========================================================== 
best                  The best trip
rapid                 A good trade off between duration, changes and constraint respect
no_train              Alternative trip without train
comfort               A trip with less changes and walking
car                   A trip with car to get to the public transport
less_fallback_walk    A trip with less walking
less_fallback_bike    A trip with less biking
less_fallback_bss     A trip with less bss
fastest               A trip with minimum duration
non_pt_walk           A trip without public transport, only walking
non_pt_bike           A trip without public transport, only biking
non_pt_bss            A trip without public transport, only bike sharing
===================== ========================================================== 

.. _odt:

On demand transportation
************************

Some transit agencies force travelers to call them to arrange a pickup at a particular place or stop point.

Besides, some stop times can be "estimated" *in data by design* :

* A standard GTFS contains only regular time: that means transport agencies should arrive on time :)
* But navitia can be fed with more specific data, where "estimated time" means that there will be
no guarantee on time respect by the agency. It often occurs in suburban or rural zone.

After all, the stop points can be standard (such as bus stop or railway station) 
or "zonal" (where agency can pick you up you anywhere, like a cab).

That's some kind of "responsive locomotion" (ɔ).

So public transport lines can mix different methods to pickup travelers:

* regular

    * line does not contain any estimated stop times, nor zonal stop point location. 
    * No need to call too.

* odt_with_stop_time

    * line does not contain any estimated stop times, nor zonal stop point location.
    * But you will have to call to take it.

* odt_with_stop_point

    * line can contain some estimated stop times, but no zonal stop point location.
    * And you will have to call to take it.

* odt_with_zone

    * line can contain some estimated stop times, and zonal stop point location.
    * And you will have to call to take it
    * well, not really a public transport line, more a cab...



