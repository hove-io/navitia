SNCF API documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. contents:: Index

Overview
========

This document describes how to use the SNCF API based on navitia via the v1 interface, and the returned resources.
Navitia is an Open Source software developed by Canal TP. (www.canaltp.fr)

The SNCF API handle to :

* compute journey from and to "station" or "administrative region"
* display next departures or arrivals
* display route schedules
* lexicographical search (autocomplete)

The SNCF API contains theorical train data for the following commercial modes : TGV, TER, Transilien, Intercités.

Read our lexicon : https://github.com/OpenTransport/vocabulary/blob/master/vocabulary.md

Authentication
================

You must authenticate to use **SNCF API**. When you register we give you an authentication key to the API.

There is two ways for authentication:

You can use a `Basic HTTP authentication`_, where the username is the key, and without password.
Username : copy / paste your key
Password : leave the field blank


The other method is to pass directly the key in the `HTTP Authorization header`_ like that:

.. code-block:: none

    Authorization: mysecretkey

.. _Basic HTTP authentication: http://tools.ietf.org/html/rfc2617#section-2
.. _HTTP Authorization header: http://tools.ietf.org/html/rfc2616#section-14.8

Endpoint
********

The only endpoint of this version of the api is : https://api.sncf.com/v1/coverage/sncf

Some easy examples
******************

* Transport mode available in the service
	* http://api.sncf.com/v1/coverage/sncf/
* Which services are available on this coverage? take a look at the links under this URL
	* https://api.navitia.io/v1/coverage/fr-idf
* Networks available?
	* https://api.navitia.io/v1/coverage/fr-idf/networks
* SNCF network lines?
	* https://api.navitia.io/v1/coverage/fr-idf/networks/network:RTP/lines 
* Too much lines, let's use physical mode filtering
	* physical modes managed by RATP 
		* https://api.navitia.io/v1/coverage/fr-idf/networks/network:RTP/physical_modes
	* metro lines 
		* https://api.navitia.io/v1/coverage/fr-idf/networks/network:RTP/physical_modes/physical_mode:Metro/lines 
* By the way, what is close to me?
	* https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/places_nearby
	* or https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/stop_points
	* or https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/lines
	* or https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/stop_schedules
	* or ...

Resources
*********

All the resources return a response containing a links object, a paging object, and the requested object.

* **Coverage** :

+---------------------------------------------------------------+--------------------------------------+
| ``get`` /coverage                                             | List of the areas covered by SNCF API|
+---------------------------------------------------------------+--------------------------------------+
| ``get`` /coverage/*region_id*                                 | Information about a specific region  |
+---------------------------------------------------------------+--------------------------------------+ 

* **Public transportation objects** : List of the public transport objects of a region

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*/*collection_name*               | Collection of objects of a region   |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*/*collection_name*/*object_id*   | Information about a specific region |
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

* **Places/Autocomplete** : Search in the datas

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/places                                      | List of objects                     |
+---------------------------------------------------------------+-------------------------------------+

* **Places nearby** : List of objects near an object or a coord

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/places_nearby               | List of objects near the resource   |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*lon;lat*/places_nearby                     | List of objects near the resource   |
+---------------------------------------------------------------+-------------------------------------+

Interface
=========
We aim to implement `HATEOAS <http://en.wikipedia.org/wiki/HATEOAS>`_ concept with Navitia.

Each response contains a linkable object and lots of links.
Links allow you to know all accessible uris and services for a given point.

Paging
======

All responses contain a paging object

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
https://api.navitia.io/v1/coverage/fr-idf/networks/network:RTP/lines/line:RTP:1197611/stop_schedules

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
no_destination             Couldn't find an destination for the journeys
no_origin_nor_destination  Couldn't find an origin nor a destination for the journeys
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

Public transportation objects
******************************

You can explore the public transportation objects 
easily with these apis. You just need to add at the end of your url 
a collection name to see all the objects of a particular collection.
To see an object add the id of this object at the end of the collection's url.
The only arguments are the ones of `paging`_.

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

Examples

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


.. _places:
Places
******

This api search in public transport objects via their names.
It returns, in addition of classic objects, a collection of `place`_.


+------------------------------------------+
| *Warning*                                |
|                                          |
| There is no pagination for this api      |
+------------------------------------------+

Parameters
##########

+---------+---------------+-----------------+----------------------------------------+--------------------------------------+
| Required| Name          | Type            | Description                            | Default value                        |
+=========+===============+=================+========================================+======================================+
| yep     | q             | string          | The search term                        |                                      |
+---------+---------------+-----------------+----------------------------------------+--------------------------------------+
| nop     | type\[\]      | array of string | Type of objects you want to query      | \[``stop_area``, ``stop_point``,     |
|         |               |                 |                                        | ``administrative_region``\]          |
+---------+---------------+-----------------+----------------------------------------+--------------------------------------+
| nop     | admin_uri\[\] | array of string | If filled, will restrained the search  |                                      |
|         |               |                 | within the given admin uris            |                                      |
+---------+---------------+-----------------+----------------------------------------+--------------------------------------+

+-------------------------------------------------------------------------+
| *Warning*                                                               |
|                                                                         |
| In the SNCF API, there are no POI and adresses.                         |
+-------------------------------------------------------------------------+

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

Places Nearby
*************

This api search for public transport object near another object, or near coordinates.
It returns, in addition of classic objects, a collection of `place`_.

+------------------------------------------+
| *Warning*                                |
|                                          |
| There is no pagination for this api      |
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


Journeys
********

This api compute journeys.

It will retrieve the next journeys from 
the selected public transport object.

To access the 'journey' api endpoint: `<https://api.navitia.io/v1/journeys?from={resource_id_1}&to={resource_id_2}&datetime={datetime}>`_ .

+-------------------------------------------------------------------------------------------------------------------------------------------------------------+
| *Note*                                                                                                                                                      |
|                                                                                                                                                             |
| The API SNCF handle computation journey from and to "station" or "administrative region"                                                                    |
+-------------------------------------------------------------------------------------------------------------------------------------------------------------+


.. _journeys_parameters:

Parameters
##########

+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| Required | Name                  | Type      | Description                               | Default value   |
+==========+=======================+===========+===========================================+=================+
| nop      | from                  | id        | The id of the departure of your journey   |                 |
|          |                       |           | If none are provided an isochrone is      |                 |
|          |                       |           | computed                                  |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | to                    | id        | The id of the arrival of your journey     |                 |
|          |                       |           | If none are provided an isochrone is      |                 |
|          |                       |           | computed                                  |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| yep      | datetime              | datetime  | A datetime                                |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | datetime_represents   | string    | Can be ``departure`` or ``arrival``.      | departure       |
|          |                       |           |                                           |                 |
|          |                       |           | If ``departure``, the request will        |                 |
|          |                       |           | retrieve journeys starting after          |                 |
|          |                       |           | datetime.                                 |                 |
|          |                       |           |                                           |                 |
|          |                       |           | If ``arrival`` it will retrieve journeys  |                 |
|          |                       |           | arriving before datetime.                 |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+
| nop      | forbidden_uris[]      | id        | If you want to avoid lines, modes,  networks, etc.|         |
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
| nop      | max_duration          | int       | Maximum duration of the journey           | 3600*24 s (24h) |
|          |                       |           |                                           |                 |
|          |                       |           | Like all duration, the unit is seconds    |                 |
+----------+-----------------------+-----------+-------------------------------------------+-----------------+

Objects
#######

Here is a typical journey, all sections are detailed below

.. image:: typical_itinerary.png


* main response

=================== ================== ===========================================================================
Field               Type               Description
=================== ================== ===========================================================================
journeys            array of journeys_ List of computed journeys
links               link_              Links related to the journeys
=================== ================== ===========================================================================


* Journey object

=================== ================== ===========================================================================
Field               Type               Description
=================== ================== ===========================================================================
_duration            int                Duration of the journey
nb_transfers        int                 Number of transfers in the journey
departure_date_time `datetime`_       Departure date and time of the journey
requested_date_time `datetime`_         Requested date and time of the journey
arrival_date_time   `datetime`_         Arrival date and time of the journey
sections            array `section`_    All the sections of the journey
from                `place <place>`_    The place from where the journey starts
to                  `<place>`_          The place from where the journey ends
links               `link`_             Links related to this journey
type                *enum* string       Used to qualified a journey. See the `journey_qualif`_ section for more information
fare                fare_               Fare of the journey (tickets and price)
tags                array of string     List of tags on the journey. The tags add additional information on the journey beside the journey type. See for example `multiple_journeys`_.
=================== ================== ===========================================================================


.. _section:

* Section object


+-------------------------+------------------------------------+----------------------------------------------------+
| Field                   | Type                               | Description                                        |
+=========================+====================================+====================================================+
| type                    | *enum* string                      | Type of the section, it can be:                    |
|                         |                                    |                                                    |
|                         |                                    | * ``public_transport``: public transport section   |
|                         |                                    | * ``street_network``: street section               |
|                         |                                    | * ``waiting``: waiting section between transport   |
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
+-------------------------+------------------------------------+----------------------------------------------------+
| id                      | string                             | Id of the section                                  |
+-------------------------+------------------------------------+----------------------------------------------------+
| duration                | int                                | Duration of this section                           |
+-------------------------+------------------------------------+----------------------------------------------------+
| from                    | `place`_                           | Origin place of this section                       |
+-------------------------+------------------------------------+----------------------------------------------------+
| to                      | place_                             | Destination place of this section                  |
+-------------------------+------------------------------------+----------------------------------------------------+
| links                   | Array of link_                     | Links related to this section                      |
+-------------------------+------------------------------------+----------------------------------------------------+
| display_informations    | display_informations_              | Useful information to display such as train number |
+-------------------------+------------------------------------+----------------------------------------------------+
| additionnal_informations| *enum* string                      | Other information. It can be:                      |
|                         |                                    |                                                    |
|                         |                                    | * ``regular``: no on demand transport (odt)        |
|                         |                                    | * ``has_date_time_estimated``: section with at     |
|                         |                                    |   least one estimated date time                    |
|                         |                                    | * ``odt_with_stop_time``: odt with                 |
|                         |                                    |   fix schedule                                     |
|                         |                                    | * ``odt_with_zone``: odt with zone                 |
+-------------------------+------------------------------------+----------------------------------------------------+
| geojson                 | `GeoJson <http://www.geojson.org>`_|                                                    |
+-------------------------+------------------------------------+----------------------------------------------------+
| transfer_type           | *enum* string                      | The type of this transfer it can be: ``walking``,  |
|                         |                                    |  ``guaranteed``, ``extension``                     |
+-------------------------+------------------------------------+----------------------------------------------------+
| stop_date_times         | Array of stop_date_time_           | List of the stop times of this section             |
+-------------------------+------------------------------------+----------------------------------------------------+
| departure_date_time     | `date_time date_time_object`_      | Date and time of departure                         |
+-------------------------+------------------------------------+----------------------------------------------------+
| arrival_date_time       | `date_time date_time_object`_      | Date and time of arrival                           |
+-------------------------+------------------------------------+----------------------------------------------------+

Route Schedules
***************

This api give you access to schedules of routes.
The response is made of an array of route_schedule, and another one of `note`_.
You can access it via that kind of url: `<https://api.navitia.io/v1/{a_path_to_a_resource}/route_schedules>`_

Parameters
##########

+----------+---------------------+-----------+------------------------------+---------------+
| Required | Name                | Type      | Description                  | Default Value |
+==========+=====================+===========+==============================+===============+
| yep      | from_datetime       | date_time | The date_time from           |               |
|          |                     |           | which you want the schedules |               |
+----------+---------------------+-----------+------------------------------+---------------+
| nop      | duration            | int       | Maximum duration in seconds  | 86400         |
|          |                     |           | between from_datetime        |               |
|          |                     |           | and the retrieved datetimes. |               |
+----------+---------------------+-----------+------------------------------+---------------+
| nop      | max_stop_date_times | int       | Maximum number of            |               |
|          |                     |           | stop_date_times per          |               |
|          |                     |           | schedule.                    |               |
+----------+---------------------+-----------+------------------------------+---------------+

Objects
#######

* route_schedule object

===================== =========================== ==============================================
Field                 Type                        Description
===================== =========================== ==============================================
display_informations  `display_informations`_     Usefull information about the route to display
Table                 table_                      The schedule table
===================== =========================== ==============================================

.. _table:

* table object

======= ================= ====================================
Field   Type              Description
======= ================= ====================================
Headers Array of header_  Informations about vehicle journeys
Rows    Array of row_     A row of the schedule
======= ================= ====================================

.. _header:

* header object

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

* row object

+------------+----------------------------------------------+---------------------------+
| Field      | Type                                         | Description               |
+============+==============================================+===========================+
| date_times | Array of `date_time <date_time_object>`_     | Array of date_time        |
+------------+----------------------------------------------+---------------------------+
| stop_point | stop_point_                                  | The stop point of the row |
+------------+----------------------------------------------+---------------------------+



Stop Schedules
**************

This api give you access to schedules of stops.
The response is made of an array of stop_schedule, and another one of `note`_.
You can access it via that kind of url: `<https://api.navitia.io/v1/{a_path_to_a_resource}/stop_schedules>`_

Parameters
##########

+----------+---------------------+-----------+------------------------------+---------------+
| Required | Name                | Type      | Description                  | Default Value |
+==========+=====================+===========+==============================+===============+
| yep      | from_datetime       | date_time | The date_time from           |               |
|          |                     |           | which you want the schedules |               |
+----------+---------------------+-----------+------------------------------+---------------+
| nop      | duration            | int       | Maximum duration in seconds  | 86400         |
|          |                     |           | between from_datetime        |               |
|          |                     |           | and the retrieved datetimes. |               |
+----------+---------------------+-----------+------------------------------+---------------+

Objects
#######

* stop_schedule object

===================== =============================================== ==============================================
Field                 Type                                            Description
===================== =============================================== ==============================================
display_informations  display_informations_                           Usefull information about the route to display
route                 route_                                          The route of the schedule
date_times            Array of `date_time <date_time_object>`_        When does a bus stops at the stop point
stop_point            stop_point_                                     The stop point of the schedule
===================== =============================================== ==============================================

Departures
**********

This api retrieves a list of departures from a datetime of a selected object.
Departures are ordered chronologically in growing order.

Parameters
##########

+----------+---------------------+-----------+------------------------------+---------------+
| Required | Name                | Type      | Description                  | Default Value |
+==========+=====================+===========+==============================+===============+
| yep      | from_datetime       | date_time | The date_time from           |               |
|          |                     |           | which you want the schedules |               |
+----------+---------------------+-----------+------------------------------+---------------+
| nop      | duration            | int       | Maximum duration in seconds  | 86400         |
|          |                     |           | between from_datetime        |               |
|          |                     |           | and the retrieved datetimes. |               |
+----------+---------------------+-----------+------------------------------+---------------+

Objects
#######

* departure object

===================== ========================= ========================================
Field                 Type                      Description
===================== ========================= ========================================
route                 route_                    The route of the schedule
stop_date_time        Array of stop_date_time_  When does a bus stops at the stop point
stop_point            stop_point_               The stop point of the schedule
===================== ========================= ========================================

Arrivals
********
This api retrieves a list of arrival from a datetime of a selected object.
Arrival are ordered chronologically in growing order.

Parameters
##########

+----------+---------------------+-----------+------------------------------+---------------+
| Required | Name                | Type      | Description                  | Default Value |
+==========+=====================+===========+==============================+===============+
| yep      | from_datetime       | date_time | The date_time from           |               |
|          |                     |           | which you want the schedules |               |
+----------+---------------------+-----------+------------------------------+---------------+
| nop      | duration            | int       | Maximum duration in seconds  | 86400         |
|          |                     |           | between from_datetime        |               |
|          |                     |           | and the retrieved datetimes. |               |
+----------+---------------------+-----------+------------------------------+---------------+

Objects
#######

* arrival object

===================== ========================= ========================================
Field                 Type                      Description
===================== ========================= ========================================
route                 route_                    The route of the schedule
stop_date_time        Array of stop_date_time_  When does a bus stops at the stop point
stop_point            stop_point_               The stop point of the schedule
===================== ========================= ========================================

Geographical Objects
********************

.. _coord:

Coord
********

====== ====== ============
Field  Type   Description
====== ====== ============
lon    float  Longitude
lat    float  Latitude
====== ====== ============

Public transport objects
********

.. _network:

Network
#######

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

+-----------------------------------------------------------------------------------------------------------+
| *Note*                                                                                                    |
|                                                                                                           |
| The fields "Code" and "Color" in this API are not available.                                              |
| The lines you will get with API do not correspond to commercial lines.                                    |
+-----------------------------------------------------------------------------------------------------------+

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
============ ===================== ==================================

.. _stop_point:
Stop Point
##########

======================= ===================== =====================================================================
Field                   Type                  Description
======================= ===================== =====================================================================
id                      string                Identifier of the line
name                    string                Name of the line
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
id                     string                      Identifier of the line
name                   string                      Name of the line
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


+-----------------------------------------------------------------------------------------------------------+
| *Note*                                                                                                    |
|                                                                                                           |
| The commercial mode available in the SNCF API :                                                           |
|                                                                                                           |
| - TGV                                                                                                     |
| - TER                                                                                                     |
| - Intercité                                                                                               |
| - Transilien                                                                                              |
+-----------------------------------------------------------------------------------------------------------+

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

.. _place:
Place
#####
A container containing either a `stop_point`_, `stop_area`_, `admin`_

===================== ============================= =================================
Field                 Type                          Description
===================== ============================= =================================
name                  string                        The name of the embedded object
id                    string                        The id of the embedded object
embedded_type         `embedded_type_place`_        The type of the embedded object
stop_point            *optional* `stop_point`_      Embedded Stop point
stop_area             *optional* `stop_area`_       Embedded Stop area
administrative_region *optional* `admin`_           Embedded administrative region
===================== ============================= =================================

.. _embedded_type_place:
Embedded type
_____________

===================== ============================================================
Value                 Description
===================== ============================================================
stop_point            a location where vehicles can pick up or drop off passengers
stop_area             a nameable zone, where there are some stop points
administrative_region a city, a district, a neighborhood
===================== ============================================================

Street network object
**********************

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

In France, cities are on the 8 level.

Other objects
*************

.. _date_time_object:
date_time
############

+--------------------------+----------------------+--------------------------------+
| Field                    | Type                 | Description                    |
+==========================+======================+================================+
| additionnal_informations | Array of String      | Other information: TODO enum   |
+--------------------------+----------------------+--------------------------------+
| date_times               | Array of String      | Date time                      |
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

========== ===================================== ============
Field      Type                                  Description
========== ===================================== ============
date_time  `date_time <date_time_object>`_       A date time
stop_point stop_point_                           A stop point
========== ===================================== ============

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
headsign        String          Train Number
=============== =============== ==================================

.. _link:
link
####

See `interface`_ section.

Special Parameters
******************

.. _datetime:
datetime
########

A date time with the format YYYYMMDDThhmmss

Misc mechanisms
***************

.. _multiple_journeys:
Multiple journeys
#################

Navitia can compute several kind of trips within a journey query.

The `RAPTOR <http://research.microsoft.com/apps/pubs/default.aspx?id=156567>`_ algorithm used in Navitia is a multi-objective algorithm. Thus it might return multiple journeys if it cannot know that one is better than the other. 
For example it cannot decide that a one hour trip with no connection is better than a 45 minutes trip with one connection (it is called the `pareto front <http://en.wikipedia.org/wiki/Pareto_efficiency>`_).

If the user ask for more journeys than the number of journeys given by RAPTOR (with the parameter ``min_nb_journeys`` or ``count``), Navitia will ask RAPTOR again, 
but for the following journeys (or the previous ones if the user asked with ``datetime_represents=arrival``).

Those journeys have the ``next`` (or ``previous``) value in their tags.


.. _journey_qualif:
Journey qualification process
#############################

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

Quota
========
The SNCF API has a rate limit according to the plan you registered for. For a developer plan (free), the rate limit is defined for a total calls of 3000 per day (per user).
For a entreprise plan, the rate limit can be made to measure according to your need.
As you reach your rate limit, your access to the service is limited on the remaining time.
Example : a re-user has made 3000 calls in 12 hours. The service will be freeze for the next 12 hours.

Connection between Paris’ train station
=======================================
The connection between train station within Paris are based on approximate duration journey.
See the table below:

+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+
|                   | Gare de Lyon | Gare d'Austerlitz  | Gare Montparnasse | Gare Saint Lazare | Gare du Nord  | Gare de l'est | Gare de Bercy |
+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+
| Gare de Lyon      |              |                    |                   |                   |               |               |               |
+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+
| Gare d'Austerlitz | 11           |                    |                   |                   |               |               |               |
+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+
| Gare Montparnasse | 25           | 21                 |                   |                   |               |               |               |
+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+
| Gare Saint Lazare | 12           | 24                 | 13                |                   |               |               |               |
+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+
| Gare du Nord      | 15           | 18                 | 20                | 21                |               |               |               |
+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+
| Gare de l'est     | 23           | 16                 | 19                | 20                | 9             |               |               |
+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+
| Gare de Bercy     | 17           | 19                 | 22                | 17                | 24            | 33            |               |
+-------------------+--------------+--------------------+-------------------+-------------------+---------------+---------------+---------------+

