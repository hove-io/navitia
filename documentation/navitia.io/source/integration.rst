Interface V1's documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Contents:

.. toctree::
   :maxdepth: 2


Overview
========

This document describes how to call the interface v1, and the returned resources.

Endpoint
********

The only endpoint of this version of the api is : https://api.navitia.io/v1/


Resources
#########
All the resources return a response containing a links object, a paging object, and the requested object.

* **Coverage** : List of the region covered by navitia

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage                                             | List of the areas covered by navitia|
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*                                 | Information about a specific region |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*lon;lat*                                   | Information about a specific region |
+---------------------------------------------------------------+-------------------------------------+

* **Public transportation objects** : List of the public transport objects of a region

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*/*collection_name*               |  Collection of objects of a region  |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*region_id*/*collection_name*/*object_id*   | Information about a specific region |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*lon;lat*/*collection_name*                 |  Collection of objects of a region  |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*lon;lat*/*collection_name*/*object_id*     | Information about a specific region |
+---------------------------------------------------------------+-------------------------------------+

* **Journeys** : Compute journeys

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/journeys                    |  List of journeys                   |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /journeys                                             |  List of journeys                   |
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
| ``get`` /coverage/places                                      |  List of objects                    |
+---------------------------------------------------------------+-------------------------------------+

* **Places nearby** : List of objects near an object or a coord

+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*resource_path*/places_nearby               | List of objects near the resource   |
+---------------------------------------------------------------+-------------------------------------+
| ``get`` /coverage/*lon;lat*/places_nearby                     | List of objects near the resource   |
+---------------------------------------------------------------+-------------------------------------+

Authentication
================

You must authenticate to use **navitia.io**. When you register we give you a authentication key to the API.

There is two ways for authentication, you can use a `Basic HTTP authentication`_, where the username is the key, and without password.

The other method is to pass directly the key in the `HTTP Authorization header`_ like that:

.. code-block:: none

    Authorization: mysecretkey

.. _Basic HTTP authentication: http://tools.ietf.org/html/rfc2617#section-2
.. _HTTP Authorization header: http://tools.ietf.org/html/rfc2616#section-14.8

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

HATEOAS
=======
Each response contains a link object. It allows you to know all the accessible urls for a given point

templated url
*************

Inner references
****************


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

The are two possible http codes :

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

Code 204
********

When your request is good but we are not able to find a journey

Redirections
============

Apis
====

Coverage
********
You can easily navigate through regions covered by navitia.io, with the coverage api.
The only arguments are the ones of `paging`_.

Public transportation objects
******************************

Once you have selected a region, you can explore the public transportation objects easily with these apis. You just need to add at the end of your url a collection name to see all the objects of a particular collection.
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

Example
#######

Response example for this request https://api.navitia.io/v1/coverage/iledefrance/physical_modes

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

Places
******

This api search in public transport objects via their names.
It returns, in addition of classic objects, a collection of `place`_.

.. warning::

    There is no pagination for this api

Parameters
##########

+---------+---------------+-----------------+----------------------------------------+-------------------------------------+
| Required| Name          | Type            | Description                            | Default value                       |
+=========+===============+=================+========================================+=====================================+
| yep     | q             | string          | The search term                        |                                     |
+---------+---------------+-----------------+----------------------------------------+-------------------------------------+
| nop     | type\[\]      | array of string | Type of objects you want to query      | \[``stop_area``, ``stop_point``,    |
|         |               |                 |                                        | ``poi``, ``adminstrative_region``\] |
+---------+---------------+-----------------+----------------------------------------+-------------------------------------+
| nop     | admin_uri\[\] | array of string | If filled, will restrained the search  |                                     |
|         |               |                 | within the given admin uris            | ""                                  |
+---------+---------------+-----------------+----------------------------------------+-------------------------------------+

Example
#######

Response example for : https://api.navitia.io/v1/coverage/iledefrance/places?q=rue

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

.. warning::
    There is no pagination for this api

Parameters
##########

+---------+---------------+-----------------+------------------------------------------+-------------------------------------+
| Required| name          | Type            | Description                              | Default value                       |
+=========+===============+=================+==========================================+=====================================+
| nop     | distance      | int             | Distance range in meters                 | 500                                 |
+---------+---------------+-----------------+------------------------------------------+-------------------------------------+
| nop     | type\[\]      | array of string | Type of objects you want to query        | \[``stop_area``, ``stop_point``,    |
|         |               |                 |                                          | ``poi``, ``adminstrative_region``\] |
+---------+---------------+-----------------+------------------------------------------+-------------------------------------+
| nop     | admin_uri\[\] | array of string | If filled, will restrained the search    | ""                                  |
|         |               |                 | within the given admin uris              |                                     |
+---------+---------------+-----------------+------------------------------------------+-------------------------------------+
| nop     | filter        | string          | Use to restrain returned objects.        |                                     |
|         |               |                 | for example: places_type.id=theater      |                                     |
+---------+---------------+-----------------+------------------------------------------+-------------------------------------+

Example
########

Response example for: https://api.navitia.io/v1/coverage/iledefrance/stop_areas/stop_area:TRN:SA:DUA8754575/places_nearby

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

If used within the coverage api, it will retrieve the next journeys from the selected public transport object or coordinates.

There are two ways to access this api.

The first one is: `<https://api.navitia.io/v1/{a_path_to_resource}/journeys>`_ it will retrieve all the journeys from the resource.

The other one, the most used, is to access the 'journey' api endpoint: `<https://api.navitia.io/v1/journeys?from={resource_id_1}&to={resource_id_2}&datetime={datetime}>`_ .

.. note::
    Navitia.io handle lot's of different data sets (regions). Some of them can overlap. For example opendata data sets can overlap with private data sets.

    When using the journeys endpoint the data set used to compute the journey is chosen using the possible datasets of the origin and the destination.

    For the moment it is not yet possible to compute journeys on different data sets, but it will one day be possible (with a cross-data-set system).

    If you want to use a specific data set, use the journey api within the data set: `<https://api.navitia.io/v1/coverage/{your_dataset}/journeys>`_


.. note::
    Neither the 'from' nor the 'to' parameter of the journey are required, but obviously one of them has to be provided.

    If only one is defined an isochrone is computed with every possible journeys from or to the point.


Parameters
##########

+----------+---------------------+-----------+-------------------------------------------+-----------------+
| Required | Name                | Type        Description                               | Default value   |
+==========+=====================+===========+===========================================+=================+
| nop      | from                | id        | The id of the departure of your journey   |                 |
|          |                     |           | If none are provided an isochrone is      |                 |
|          |                     |           | computed                                  |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | to                  | id        | The id of the arrival of your journey     |                 |
|          |                     |           | If none are provided an isochrone is      |                 |
|          |                     |           | computed                                  |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| yep      | datetime            | datetime  | A datetime                                |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | datetime_represents | string    | Can be ``departure`` or ``arrival``.      | departure       |
|          |                     |           |                                           |                 |
|          |                     |           | If ``departure``, the request will        |                 |
|          |                     |           | retrieve journeys starting after          |                 |
|          |                     |           | datetime.                                 |                 |
|          |                     |           |                                           |                 |
|          |                     |           | If ``arrival`` it will retrieve journeys  |                 |
|          |                     |           | arriving before datetime.                 |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | forbidden_uris[]    | id        | If you want to avoid lines, modes ...     |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | first_section_mode[]| array of  | Force the first section mode if the first | walking         |
|          |                     | string    | section is not a public transport one.    |                 |
|          |                     |           | It takes one the following values:        |                 |
|          |                     |           | ``walking``, ``car``, ``bike``, ``bss``   |                 |
|          |                     |           |                                           |                 |
|          |                     |           | bss stands for bike sharing system        |                 |
|          |                     |           |                                           |                 |
|          |                     |           | It's an array, you can give multiple      |                 |
|          |                     |           | modes                                     |                 |
|          |                     |           |                                           |                 |
|          |                     |           | Note: choosing ``bss`` implicitly allows  |                 |
|          |                     |           | the ``walking`` mode since you might have |                 |
|          |                     |           | to walk to the bss station                |                 |
|          |                     |           |                                           |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | last_section_mode[] | array of  | Same as first_section_mode but for the    | walking         |
|          |                     | string    | last section                              |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | max_duration_to_pt  | int       | Maximum allowed duration to reach the     | 15*60 s         |
|          |                     |           | public transport                          |                 |
|          |                     |           |                                           |                 |
|          |                     |           | Use this to limit the walking/biking part |                 |
|          |                     |           |                                           |                 |
|          |                     |           | Unit is seconds                           |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | walking_speed       | float     | Walking speed for the fallback sections   | 1.12 m/s        |
|          |                     |           |                                           |                 |
|          |                     |           | Speed unit must be in meter/seconds       | (4 km/h)        |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | bike_speed          | float     | Biking speed for the fallback sections    | 4.1 m/s         |
|          |                     |           |                                           |                 |
|          |                     |           | Speed unit must be in meter/seconds       | (14.7 km/h)     |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | bss_speed           | float     | Speed while using a bike from a bike      | 4.1 m/s         |
|          |                     |           | sharing system for the fallback sections  | (14.7 km/h)     |
|          |                     |           |                                           |                 |
|          |                     |           | Speed unit must be in meter/seconds       |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | car_speed           | float     | Driving speed for the fallback sections   | 16.8 m/s        |
|          |                     |           |                                           |                 |
|          |                     |           | Speed unit must be in meter/seconds       | (60 km/h)       |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | min_nb_journeys     | int       | Minimum number of different suggested     |                 |
|          |                     |           | trips                                     |                 |
|          |                     |           |                                           |                 |
|          |                     |           | More in `multiple_journeys`_              |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | max_nb_journeys     | int       | Maximum number of different suggested     |                 |
|          |                     |           | trips                                     |                 |
|          |                     |           |                                           |                 |
|          |                     |           | More in `multiple_journeys`_              |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | count               | int       | Fixed number of different journeys        |                 |
|          |                     |           |                                           |                 |
|          |                     |           | More in `multiple_journeys`_              |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | type                | string    | Allows you to filter the type of journeys |                 |
|          |                     |           |                                           |                 |
|          |                     |           | More in `journey_qualif`_                 |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | max_nb_tranfers     | int       | Maximum of number transfers               | 10              |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | disruption_active   | boolean   | If true the algorithm take the disruptions| False           |
|          |                     |           | into account, and thus avoid disrupted    |                 |
|          |                     |           | public transport                          |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | max_duration        | int       | Maximum duration of the journey           | 3600*24 s (24h) |
|          |                     |           |                                           |                 |
|          |                     |           | Like all duration, the unit is seconds    |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | wheelchair          | boolean   | If true the traveler is considered to     | False           |
|          |                     |           | be using a wheelchair, thus only          |                 |
|          |                     |           | accessible public transport are used      |                 |
|          |                     |           |                                           |                 |
|          |                     |           | be warned: many data are currently too    |                 |
|          |                     |           | faint to provide acceptable answers       |                 |
|          |                     |           | with this parameter on                    |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | show_codes          | boolean   | If true add internal id in the response   | False           |
+----------+---------------------+-----------+-------------------------------------------+-----------------+
| nop      | debug               | boolean   | Debug mode                                | False           |
|          |                     |           |                                           |                 |
|          |                     |           | No journeys are filtered in this mode     |                 |
+----------+---------------------+-----------+-------------------------------------------+-----------------+

Objects
#######

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
duration            int                Duration of the journey
nb_transfers        int                Number of transfers in the journey
departure_date_time datetime_          Departure date and time of the journey
requested_date_time datetime_          Requested date and time of the journey
arrival_date_time   datetime_          Arrival date and time of the journey
sections            array of section_  All the sections of the journey
from                place_             The place from where the journey starts
to                  place_             The place from where the journey ends
links               link_              Links related to this journey
type                *enum* string      Used to qualified a journey. See the `journey_qualif`_ section for more information
fare                fare_              Fare of the journey (tickets and price)
tags                array of string    List of tags on the journey. The tags add additional information on the journey beside the journey type. See for example `multiple_journeys`_.
=================== ================== ===========================================================================


+-----------------------------------------------------------------------------------------------------------+
| Note                                                                                                      |
| When used with just a "from" or a "to" parameter, it will not contain any sections                        |
+-----------------------------------------------------------------------------------------------------------+

.. _section:

* Section object


+--------------------------+--------------------------------------+--------------------------------------------------------+
| Field                    | Type                                 | Description                                            |
+==========================+======================================+========================================================+
| type                     | *enum* string                        | Type of the section, it can be:                        |
|                          |                                      |                                                        |
|                          |                                      | * ``PUBLIC_TRANSPORT``: public transport section       |
|                          |                                      |                                                        |
|                          |                                      | * ``STREET_NETWORK``: street section                   |
|                          |                                      |                                                        |
|                          |                                      | * ``WAITING``: waiting section between transport       |
|                          |                                      |                                                        |
|                          |                                      | * ``TRANSFER``: transfert section                      |
|                          |                                      |                                                        |
|                          |                                      | * ``ON_DEMAND_TRANSPORT``: on demand transport section |
|                          |                                      |   (odt)                                                |
|                          |                                      |                                                        |
|                          |                                      | * ``boarding``: boarding on plane                      |
|                          |                                      |                                                        |
|                          |                                      | * ``landing``: landing off the plane                   |
|                          |                                      |                                                        |
|                          |                                      | * ``BSS_RENT``: taking a bike from a bike sharing      |
|                          |                                      |   system (bss)                                         |
|                          |                                      |                                                        |
|                          |                                      | * ``BSS_PUT_BACK``: putting back a bike from a bike    |
|                          |                                      |   sharing system (bss)                                 |
|                          |                                      |                                                        |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| id                       | string                               | Id of the section                                      |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| mode                     | *enum* string                        | Mode of the street network: ``Walking``, ``Bike``,     |
|                          |                                      | ``Car``                                                |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| duration                 | int                                  | Duration of this section                               |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| from                     | place_                               | Origin place of this section                           |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| to                       | place_                               | Destination place of this section                      |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| links                    | Array of link_                       | Links related to this section                          |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| display_informations     | display_informations_                | Useful information to display                          |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| additionnal_informations | *enum* string                        | Other information. It can be:                          |
|                          |                                      |                                                        |
|                          |                                      | * ``regular``: no on demand transport (odt)            |
|                          |                                      |                                                        |
|                          |                                      | * ``has_date_time_estimated``: section with at least   |
|                          |                                      |   one estimated date time                              |
|                          |                                      |                                                        |
|                          |                                      | * ``odt_with_stop_time``: odt with                     |
|                          |                                      |   fix schedule                                         |
|                          |                                      |                                                        |
|                          |                                      | * ``odt_with_zone``: odt with zone                     |
|                          |                                      |                                                        |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| geojson                  | `GeoJson <http://www.geojson.org>`_  |                                                        |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| path                     | Array of path_                       | The path of this section                               |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| transfer_type            | *enum* string                        | The type of this transfer it can be: ``WALKING``,      |
|                          |                                      | ``GUARANTEED``, ``EXTENSION``                          |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| stop_date_times          | Array of stop_date_time_             | List of the stop times of this section                 |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| departure_date_time      | `date_time <date_time_object>`_      | Date and time of departure                             |
+--------------------------+--------------------------------------+--------------------------------------------------------+
| arrival_date_time        | `date_time <date_time_object>`_      | Date and time of arrival                               |
+--------------------------+--------------------------------------+--------------------------------------------------------+


.. _path:

* Path object

  A path object in composed of an array of path_item_ (segment).

.. _path_item:

* Path item object

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

* Fare object

===================== =========================== ===================================================================
Field                 Type                        Description
===================== =========================== ===================================================================
total                 cost_                       total cost of the journey
found                 boolean                     False if no fare has been found for the journey, True otherwise
links                 link_                       Links related to this object. Link with related `tickets <ticket>`_
===================== =========================== ===================================================================

.. _cost:

* Cost object

===================== =========================== =============
Field                 Type                        Description
===================== =========================== =============
value                 float                       cost
currency              string                      currency
===================== =========================== =============

.. _ticket:

* Ticket object 

===================== =========================== ========================================
Field                 Type                        Description
===================== =========================== ========================================
id                    string                      Id of the ticket    
name                  string                      Name of the ticket
found                 boolean                     False if unknown ticket, True otherwise
cost                  cost_                       Cost of the ticket
links                 array of link_              Link to the section_ using this ticket
===================== =========================== ========================================


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
| stop_point | `stop_point`_                                | The stop point of the row |
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

Objects
=======

Geographical Objects
********************

.. _coord:

Coord
#####

====== ====== ============
Field  Type   Description
====== ====== ============
lon    float  Longitude
lat    float  Latitude
====== ====== ============

Public transport objects
************************

.. _network:

Network
#######

====== ============= ==========================
Field  Type          Description
====== ============= ==========================
id     string        Identifier of the network
name   string        Name of the network
coord  `coord`_      Center of the network
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
============ ===================== ==================================

.. _stop_point:

Stop Point
##########

===================== ===================== =====================================================================
Field                 Type                  Description
===================== ===================== =====================================================================
id                    string                Identifier of the line
name                  string                Name of the line
coord                 `coord`_              Coordinates of the stop point
adminstrative_regions array of `admin`_     Administrative regions of the stop point in which is the stop point
equipments            array of string       Equipments of the stop point
stop_area             `stop_area`_          Stop Area containing this stop point
===================== ===================== =====================================================================

.. _stop_area:

Stop Area
#########

===================== =========================== ==================================================================
Field                 Type                        Description
===================== =========================== ==================================================================
id                    string                      Identifier of the line
name                  string                      Name of the line
coord                 `coord`_                    Coordinates of the stop area
adminstrative_regions array of `admin`_           Administrative regions of the stop area in which is the stop area
equipments            array of string             Equipments of the stop area
stop_points           array of `stop_point`_      Stop points contained in this stop area
===================== =========================== ==================================================================


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
A container containing either a `stop_point`_, :ref:`stop_area`, :ref:`address`, , :ref:`poi`, :ref:`admin`

==================== ============================= =================================
Field                Type                          Description
==================== ============================= =================================
name                 string                        The id of the embedded object
id                   string                        The name of the embedded object
embedded_type        `embedded_type_place`_        The type of the embedded object
stop_point           *optional* `stop_point`_      Embedded Stop point
stop_area            *optional* `stop_area`_       Embedded Stop area
address              *optional* `address`_         Embedded address
poi                  *optional* `poi`_             Embedded poi
adminstrative_region *optional* `admin`_           Embedded administrative region
==================== ============================= =================================

.. _embedded_type_place:

Embedded type
_____________

==================== =================
Value                Description
==================== =================
stop_point
stop_area
address
poi
adminstrative_region
==================== =================

Street network objects
**********************

.. _poi:

Poi
###

================ ================================== =======================================
Field            Type                               Description
================ ================================== =======================================
id               string                             Identifier of the poi type
name             string                             Name of the poi type
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

===================== =========================== ==================================================================
Field                 Type                        Description
===================== =========================== ==================================================================
id                    string                      Identifier of the address
name                  string                      Name of the address
coord                 `coord`_                    Coordinates of the address
house_number          int                         House number of the address
adminstrative_regions array of `admin`_           Administrative regions of the address in which is the stop area
===================== =========================== ==================================================================

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
equipments      Array of String
=============== =============== ==================================

.. _link:

link
####



Special Parameters
******************

.. _datetime:

datetime
########

A date time with the format YYYYMMDDTHHMMSS

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


