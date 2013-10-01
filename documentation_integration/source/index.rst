Welcome to Interface V1's documentation!
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Contents:

.. toctree::
   :maxdepth: 2


Overview
========

This document describes how to call the interface v1, and the returned resources.

Endpoint
********

The only endpoint of this version of the api is : http://api.navitia.io/v1/


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

Authentification
================

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
#######

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

* Code 404 :

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

    When you're request is good but we are not able to find a journey

Redirections
============

Apis
====

Coverage
********
You can easily navigate through regions covered by navitia.io, with the coverage api.
The only arguments are the ones of :ref:`paging`.

Public transportation objects
******************************

Once you've selected a region, you can explore the public transportation objects easily with these apis. You just need to add at the end of your url a collection name to see all the objects of a particular collection.
To see an object add the id of this object at the end of the collection's url.
The only arguments are the ones of :ref:`paging`.

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

Response example for this request ``http://api.navitia.io/v1/coverage/paris/physical_modes``

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
It returns, in addition of classic objects, a collection of :ref:`place`.

.. warning::
    There is no pagination for this api

Parameters
##########

+---------+---------------+-----------------+----------------------------------------+-------------------------------------+
| Required| Parameter     | Type            | Description                            | Default value                       |
+=========+===============+=================+========================================+=====================================+
| yep     | q             | string          | The search term                        |                                     |
+---------+---------------+-----------------+----------------------------------------+-------------------------------------+
| nop     | type\[\]      | array of string | Type of objects you want to query      | \[``stop_area``, ``stop_point``,    |
|         |               |                 |                                        | ``poi``, ``adminstrative_region``\] |
+---------+---------------+-----------------+----------------------------------------+-------------------------------------+
| nop     | admin_uri\[\] | array of string | If filled, will restrained the search  |                                     |
|         |               |                 | within the given admin uris            | ""                                  |
+---------+---------------+-----------------+----------------------------------------+-------------------------------------+

file:///home/vlara/workspace/documentation_v1/build/html/index.html
Example
#######

Response example for : ``http://api.navitia.io/v1/coverage/paris/places?q=rue``

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
It returns, in addition of classic objects, a collection of :ref:`place`.

.. warning::
    There is no pagination for this api

Parameters
##########

+---------+---------------+-----------------+------------------------------------------+-------------------------------------+
| Required| Parameter     | Type            | Description                              | Default value                       |
+=========+===============+=================+==========================================+=====================================+
| nop     | distance      | int             | Distance range in meters                 | 500                                 |
+---------+---------------+-----------------+------------------------------------------+-------------------------------------+
| nop     | type\[\]      | array of string | Type of objects you want to query        | \[``stop_area``, ``stop_point``,    |
|         |               |                 |                                          | ``poi``, ``adminstrative_region``\] |
+---------+---------------+-----------------+------------------------------------------+-------------------------------------+
| nop     | admin_uri\[\] | array of string | If filled, will restrained the search    | ""                                  |
|         |               |                 | within the given admin uris              |                                     |
+---------+---------------+-----------------+------------------------------------------+-------------------------------------+

Example
########

Response example for : ``http://api.navitia.io/v1/coverage/paris/places_nearby?uri=stop_area:TAN:SA:RUET``

.. code-block:: json

    {
    "places_nearby": [
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


Journeys
********

This api compute journeys.
If used within the coverage api, it will retrieve the next journeys from the selected public transport object or coordinates.

Parameters
##########

Objects
#######

* Journey object

=================== ======================= ======================================================================
Field               Type                    Description
=================== ======================= ======================================================================
duration            int                     Duration of the journey
nb_transfers        int                     Number of transfers in the journey
departure_date_time :ref:`datetime`         Departure date and time of the journey
requested_date_time :ref:`datetime`         Requested date and time of the journey
arrival_date_time   :ref:`datetime`         Arrival date and time of the journey
sections            array of :ref:`section` All the sections of the journey
from                :ref:`place`            The place from where the journey starts
to                  :ref:`place`            The place from where the journey starts
links               :ref:`link`             Links related to this journey
type                *enum* string           Use to qualified a journey can be ``comfort``, ``rapid``, ``healthy``
=================== ======================= ======================================================================

.. note::
    When used with just a from or a to parameter, it will not contain the sections parameter nor the from (if the from parametre was given).


* Section object


+--------------------------+------------------------------------+--------------------------------------------------------+
| Field                    | Type                               | Description                                            |
+==========================+====================================+========================================================+
| type                     | *enum* string                      | Type of the section, it can be : ``PUBLIC_TRANSPORT``, |
|                          |                                    | ``STREET_NETWORK``, ``WAITING``, ``TRANSFER``          |
+--------------------------+------------------------------------+--------------------------------------------------------+
| mode                     | *enum* string                      | Mode of the street network : ``Walking``, ``Bike``,    |
|                          |                                    | ``Car``, ``br``                                        |
+--------------------------+------------------------------------+--------------------------------------------------------+
| duration                 | int                                |  Duration of this section                              |
+--------------------------+------------------------------------+--------------------------------------------------------+
| from                     | :ref:`place`                       |  Origin place of this section                          |
+--------------------------+------------------------------------+--------------------------------------------------------+
| to                       | :ref:`place`                       |  Destination place of this section                     |
+--------------------------+------------------------------------+--------------------------------------------------------+
| links                    | Array of :ref:`link`               |  Links related to this section                         |
+--------------------------+------------------------------------+--------------------------------------------------------+
| display_informations     | :ref:`display_informations`        |  Usefull information to display                        |
+--------------------------+------------------------------------+--------------------------------------------------------+
| additionnal_informations | *enum* string                      |  Other informations : TODO                             |
+--------------------------+------------------------------------+--------------------------------------------------------+
| geojson                  | `GeoJson <http://www.geojson.org>` |                                                        |
+--------------------------+------------------------------------+--------------------------------------------------------+
| path                     | Array of :ref:`path`               | The path of this section                               |
+--------------------------+------------------------------------+--------------------------------------------------------+
| transfer_type            | *enum* string                      | The type of this transfer it can be : ``WALKING``,     |
|                          |                                    | ``GUARANTEED``, ``EXTENSION``                          |
+--------------------------+------------------------------------+--------------------------------------------------------+
| stop_date_times          | Array of :ref:`stop_date_time`     | List of the stop times of this section                 |
+--------------------------+------------------------------------+--------------------------------------------------------+
| departure_date_time      | :ref:`stop_date_time`              | Date and time of departure                             |
+--------------------------+------------------------------------+--------------------------------------------------------+
| arrival_date_time        | :ref:`stop_date_time`              | Date and time of arrival                               |
+--------------------------+------------------------------------+--------------------------------------------------------+


Route Schedules
***************

Stop Schedules
**************

Departures
**********

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
coord  :ref:`coord`  Center of the network
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
routes          array of :ref:`route`  Routes of the line
commercial_mode :ref:`commercial_mode` Commercial mode of the line
=============== ====================== ============================

.. _route:

Route
#####

============ ===================== ==================================
Field        Type                  Description
============ ===================== ==================================
id           string                Identifier of the route
name         string                Name of the route
is_frequence bool                  Is the route has frequence or not
line         :ref:`line`           The line of this route
============ ===================== ==================================

.. _stop_point:

Stop Point
##########

===================== ===================== =====================================================================
Field                 Type                        Description
===================== ===================== =====================================================================
id                    string                Identifier of the line
name                  string                Name of the line
coord                 :ref:`coord`          Coordinates of the stop point
adminstrative_regions array of :ref:`admin` Administrative regions of the stop point in which is the stop point
equipments            array of string       Equipments of the stop point
stop_area             :ref:`stop_area`      Stop Area containing this stop point
===================== ===================== =====================================================================

.. _stop_area:

Stop Area
#########

===================== =========================== ==================================================================
Field                 Type                        Description
===================== =========================== ==================================================================
id                    string                      Identifier of the line
name                  string                      Name of the line
coord                 :ref:`coord`                Coordinates of the stop area
adminstrative_regions array of :ref:`admin`       Administrative regions of the stop area in which is the stop area
equipments            array of string             Equipments of the stop area
stop_points           array of :ref:`stop_point`  Stop points contained in this stop area
===================== =========================== ==================================================================


.. _commercial_mode:

Commercial Mode
###############

================ =============================== =======================================
Field            Type                            Description
================ =============================== =======================================
id               string                          Identifier of the commercial mode
name             string                          Name of the commercial mode
physical_modes   array of :ref:`physical_mode`   Physical modes of this commercial mode
================ =============================== =======================================

.. _physical_mode:

Physical Mode
#############

==================== ================================ ========================================
Field                Type                             Description
==================== ================================ ========================================
id                   string                           Identifier of the physical mode
name                 string                           Name of the physical mode
commercial_modes     array of :ref:`commercial_mode`  Commercial modes of this physical mode
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
A container containing either a :ref:`stop_point`, :ref:`stop_area`, :ref:`address`, , :ref:`poi`, :ref:`admin`

==================== ============================= =================================
Field                Type                          Description
==================== ============================= =================================
name                 string                        The id of the embedded object
id                   string                        The name of the embedded object
embedded_type        :ref:`embedded_type_place`    The type of the embedded object
stop_point           *optional* :ref:`stop_point`  Embedded Stop point
stop_area            *optional* :ref:`stop_area`   Embedded Stop area
address              *optional* :ref:`address`     Embedded address
poi                  *optional* :ref:`poi`         Embedded poi
adminstrative_region *optional* :ref:`admin`       Embedded adminstrative region
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
poi_type         :ref:`poi_type`                    Type of the poi
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
coord                 :ref:`coord`                Coordinates of the adress
house_number          int                         House number of the address
adminstrative_regions array of :ref:`admin`       Administrative regions of the address in which is the stop area
===================== =========================== ==================================================================

.. _admin:

Administrative region
#####################


===================== =========================== ==================================================================
Field                 Type                        Description
===================== =========================== ==================================================================
id                    string                      Identifier of the address
name                  string                      Name of the address
coord                 :ref:`coord`                Coordinates of the adress
level                 int                         Level of the admin
zip_code              string                      Zip code of the admin
===================== =========================== ==================================================================
