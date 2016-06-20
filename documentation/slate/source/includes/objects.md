Objects
=======

Standard Objects
----------------

### Coord

Lots of object are geographically localized :

|Field|Type|Description|
|-----|----|-----------|
|lon|float|Longitude|
|lat|float|Latitude|

### Iso-date-time

Navitia manages every date or time as a UTC date-time. The web-service 

-   exposes every date times as local times via an ISO 8601 "YYYYMMDDThhmmss" string
-   can be request using local times via an ISO 8601 "YYYYMMDDThhmmss" string

For example: <https://api.navitia.io/v1/journeys?from=bob&to=bobette&datetime=20140425T1337>

There are lots of ISO 8601 libraries in every kind of language that you should use before breaking down <https://youtu.be/-5wpm-gesOY>


Public transport objects
------------------------

### Network

``` json
{
    "id":"network:RAT:1",
    "name":"RATP"
}
```

Networks are fed by agencies in GTFS format.

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the network|
|name|string|Name of the network|

### Line

``` json
{
    "id":"line:RAT:M6",
    "name":"Nation - Charles de Gaule Etoile",
    "code":"6",
    "color":"79BB92",
    "opening_time":"053000",
    "closing_time":"013600",
    "routes":[
        {"...": "..."}
    ],
    "commercial_mode":{
        "id":"commercial_mode:Metro",
        "name":"Metro"
    },
    "physical_modes":[
        {
            "name":"Métro",
            "id":"physical_mode:Metro"
        }
    ]
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the line|
|name|string|Name of the line|
|code|string|Code name of the line|
|color|string|Color of the line|
|opening_time|string|Opening hour at format HHMMSS|
|closing_time|string|Closing hour at format HHMMSS|
|routes|array of [route](#route)|Routes of the line|
|commercial_mode|[commercial_mode](#commercial-mode)|Commercial mode of the line|
|physical_mode|array of [physical_mode](#physical-mode)|Physical modes of the line|

### Route

``` json
{
    "id":"route:RAT:M6",
    "name":"Nation - Charles de Gaule Etoile",
    "is_frequence":"False",
    "line":{
        "id":"line:RAT:M6",
        "name":"Nation - Charles de Gaule Etoile",
        "...": "..."
    },
    "direction":{
        "id":"stop_area:RAT:SA:GAUET",
        "name":"Charles de Gaulle - Etoile (Paris)",
        "...": "..."
    }
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the route|
|name|string|Name of the route|
|is_frequence|bool|Is the route has frequency or not|
|line|[line](#line)|The line of this route|
|direction|[place](#place)|The direction of this route|

As "direction" is a [place](#place) , it can be a poi in some data.

### <a name="stop-point"></a>Stop Point

``` json
{
    "id":"stop_point:RAT:SP:GARIB2",
    "name":"Garibaldi",
    "label":"Garibaldi (Saint-Ouen)",
    "coord":{
        "lat":"48.906032",
        "lon":"2.331733"
    },
    "administrative_regions":[{"...": "..."}],
    "equipments":[{"...": "..."}],
    "stop_area":{"...": "..."}
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the stop point|
|name|string|Name of the stop point|
|coord|[coord](#coord)|Coordinates of the stop point|
|administrative_regions|array of [admin](#admin)|Administrative regions of the stop point in which is the stop point|
|equipments|array of string|list of [equipment](#equipment) of the stop point|
|stop_area|[stop_area](#stop-area)|Stop Area containing this stop point|

### <a name="stop-area"></a>Stop Area

``` json
{
    "id":"stop_area:RAT:SA:GAUET",
    "name":"Charles de Gaulle - Etoile",
    "label":"Charles de Gaulle - Etoile (Paris)",
    "coord":{
        "lat":"48.874408",
        "lon":"2.295763"
    },
    "administrative_regions":[
        {"...": "..."}
    ]
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the stop area|
|name|string|Name of the stop area|
|coord|[coord](#coord)|Coordinates of the stop area|
|administrative_regions|array of [admin](#admin)|Administrative regions of the stop area in which is the stop area|
|stop_points|array of [stop_point](#stop-point)|Stop points contained in this stop area|

### <a name="commercial-mode"></a>Commercial Mode

``` json
{
    "id":"commercial_mode:Metro",
    "name":"Metro"
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the commercial mode|
|name|string|Name of the commercial mode|
|physical_modes|array of [physical_mode](#physical-mode)|Physical modes of this commercial mode|

### <a name="physical-mode"></a>Physical Mode

``` json
{
    "id":"physical_mode:Tramway",
    "name":"Tramway"
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the physical mode|
|name|string|Name of the physical mode|
|commercial_modes|array of [commercial_mode](#commercial-mode)|Commercial modes of this physical mode|

Physical modes are fastened and normalized. If you want to propose modes
filter in your application, you should use [physical_mode](#physical-mode) rather than
[commercial_mode](#commercial-mode).

Here is the valid id list:

-   physical_mode:Air
-   physical_mode:Boat
-   physical_mode:Bus
-   physical_mode:BusRapidTransit
-   physical_mode:Coach
-   physical_mode:Ferry
-   physical_mode:Funicular
-   physical_mode:LocalTrain
-   physical_mode:LongDistanceTrain
-   physical_mode:Metro
-   physical_mode:RapidTransit
-   physical_mode:Shuttle
-   physical_mode:Taxi
-   physical_mode:Train
-   physical_mode:Tramway

You can use these ids in the forbidden_uris[] parameter from
[journeys parameters](#journeys-parameters) for exemple.

### <a name="company"></a>Company

``` json
{
    "id": "company:RAT:1",
    "name": "RATP"
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the company|
|name|string|Name of the company|

### Place

A container containing either a [admin](#admin), [poi](#poi), [address](#address), [stop_area](#stop-area),
[stop_point](#stop-point)

``` json
{
    "id": "admin:2191338",
    "name": "Quartier des Épinettes (75017)",
    "quality": 70,
    "embedded_type": "administrative_region",
    "administrative_region": {
        "...": "..."
    }
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|The id of the embedded object|
|name|string|The name of the embedded object|
|quality|*optional* integer|The quality of the place|
|embedded_type|[embedded_type](#embedded-type)|The type of the embedded object|
|administrative_region|*optional* [admin](#admin)|Embedded administrative region|
|stop_area|*optional* [stop_area](#stop-area)|Embedded Stop area|
|poi|*optional* [poi](#poi)|Embedded poi|
|address|*optional* [address](#address)|Embedded address|
|stop_point|*optional* [stop_point](#stop-point)|Embedded Stop point|


### Trip

A trip corresponds to a scheduled train circulation (and all its linked real-time informations).

An example : the train doing Paris-Marseille every day at 19h30 is the Trip named 456789.

``` json
{
    "id": "OIF:67308746-10-1",
    "name": "67308746"
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|The id of the trip|
|name|string|The name of the trip|


### <a name="pt-object"></a>Pt_object

A container containing either a [network](#network), [commercial_mode](#commercial-mode), [line](#line), [route](#route),
[stop_area](#stop_point), [stop_area](#stop_point), [trip](#trip)

``` json
{
    "id": "OCE:SN009862F01013",
    "name": "OCE:SN009862F01013",
    "quality": 0,
    "embedded_type": "trip",
    "trip": {
        "id": "OCE:SN009862F01013",
        "name": "9862"
    }
}
```

|Field|Type|Description|
|-----|----|-----------|
|id|string|The id of the embedded object|
|name|string|The name of the embedded object|
|quality|*optional* integer|The quality of the object|
|embedded_type|[embedded_type](#embedded-type)|The type of the embedded object|
|stop_area|*optional* [stop_area](#stop-area)|Embedded Stop area|
|stop_point|*optional* [stop_point](#stop-point)|Embedded Stop point|
|network|*optional* [network](#network)|Embedded network|
|commercial_mode|*optional* [commercial_mode](#commercial-mode)|Embedded commercial_mode|
|stop_area|*optional* [stop_area](#stop-area)|Embedded Stop area|
|line|*optional* [line](#line)|Embedded line|
|route|*optional* [route](#route)|Embedded route|
|trip|*optional* [trip](#trip)|Embedded trip|

Real time and disruption objects
--------------------------------

### Disruption

``` json
{
    "id": "ce7e265d-5762-45b6-ab4d-a1df643dd48d",
    "status": "active",
    "disruption_id": "ce7e265d-5762-45b6-ab4d-a1df643dd48d",
    "impact_id": "ce7e265d-5762-45b6-ab4d-a1df643dd48d",
    "severity": {
        "name": "trip delayed",
        "effect": "SIGNIFICANT_DELAYS"
    },
    "application_periods": [
        {
            "begin": "20160608T215400",
            "end": "20160608T230959"
        }
    ],
    "messages": [
        {"text": "Strike"}
    ],
    "updated_at": "20160617T132624",
    "impacted_objects": [
        {"...": "..."}
    ],
    "cause": "Cause..."
}
```

|Field | Type | Description |
|------|------|-------------|
|id     | string                                |Id of the disruption
|status | between: "past", "active" or "future" |state of the disruption
|disruption_id | string                         |for traceability: Id of original input disruption
|impact_id     | string                         |for traceability: Id of original input impact
|severity      | [severity](#severity)          |gives some categorization element
|application_periods |array of [period](#period)       |dates where the current disruption is active
|messages            |[message](#message)     |text to provide to the traveler
|updated_at          |[iso-date-time](#iso-date-time) |date_time of last modifications 
|impacted_objects    |array of [pt_object](#pt_object) |The list of public transport objects which are affected by the disruption
|cause               |string                   |why is there such a disruption?

### Message

``` json
{
    "text": "Strike",
    "channel": {
        "...": "..."
    }
}
```

|Field|Type|Description|
|-----|----|-----------|
|text|string|a message to bring to a traveler|
|channel|[channel](#channel)|destination media. Be careful, no normalized enum for now|

### Severity

Severity object can be used to make visual grouping.

``` json
{
    "color": "#FF0000",
    "priority": 42,
    "name": "trip delayed",
    "effect": "SIGNIFICANT_DELAYS"
}
```

| Field         | Type         | Description                                    |
|---------------|--------------|------------------------------------------------|
| color         | string     | HTML color for classification                  |
| priority      | integer    | given by the agency : 0 is strongest priority. it can be null |
| name          | string     | name of severity                               |
| effect        | Enum       | Normalized value of the effect on the public transport object See the GTFS RT documentation at <https://developers.google.com/transit/gtfs-realtime/reference#Effect> |

### Channel

``` json
{
    "id": "rt",
    "content_type": "text/html",
    "name": "rt",
    "types": [
        "web",
        "mobile"
    ]
}
```

| Field         | Type       | Description
|---------------|------------|---------------------------------------------
| id            |string      |Identifier of the address
| content_type  |string      |Like text/html, you know? Otherwise, take a look at <http://www.w3.org/Protocols/rfc1341/4_Content-Type.html>
| name          |string      |name of the Channel

### Period

``` json
{
    "begin": "20160608T215400",
    "end": "20160608T230959"
}
```

|Field|Type|Description|
|-----|----|-----------|
|begin|[iso-date-time](#iso-date-time)|Beginning date and time of an activity period|
|end|[iso-date-time](#iso-date-time)|Closing date and time of an activity period|


Street network objects
----------------------

### Poi Type
``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/poi_types' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'
```

`/poi_types` lists groups of point of interest. You will find classifications as theater, offices or bicycle rental station for example.

 
|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the poi type|
|name|string|Name of the poi type|

### <a name="poi"></a>Poi
``` shell
#useless request, with huge response
$ curl 'https://api.navitia.io/v1/coverage/sandbox/pois' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#inverted geocoding request, more usable
$ curl 'https://api.navitia.io/v1/coverage/sandbox/coords/2.377310;48.847002/pois' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#very smart request
#combining filters to get some specific POIs, as bicycle rental stations, 
#nearby a coordinate
$ curl 'https://api.navitia.io/v1/coverage/sandbox/poi_types/poi_type:amenity:bicycle_rental/coords/2.377310;48.847002/pois' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'
```

Poi = Point Of Interest

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the poi|
|name|string|Name of the poi|
|poi_type|[poi_type](#poi-type)|Type of the poi|

### Address

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the address|
|name|string|Name of the address|
|coord|[coord](#coord)|Coordinates of the address|
|house_number|int|House number of the address|
|administrative_regions|array of [admin](#admin)|Administrative regions of the address in which is the stop area|

### Administrative region

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the address|
|name|string|Name of the address|
|coord|[coord](#coord)|Coordinates of the address|
|level|int|Level of the admin|
|zip_code|string|Zip code of the admin|

Cities are mainly on the 8 level, dependant on the country
(<http://wiki.openstreetmap.org/wiki/Tag:boundary%3Dadministrative>)


Other objects
-------------

### pt-date-time

pt-date-time (pt stands for "public transport") is a complex date time object to manage the difference between stop and leaving times at a stop.

|Field                    | Type               | Description
|-------------------------|--------------------|-----------------------------
|additionnal_informations | Array of String    | Other information: TODO enum
|departure_date_time      | [iso-date-time](#iso-date-time)    | A date time
|arrival_date_time        | [iso-date-time](#iso-date-time)    | A date time
|links                    | Array of [link](#link) |internal links to notes

### note

|Field|Type|Description|
|-----|----|-----------|
|id|String|id of the note|
|value|String|The content of the note|

### stop_date_time

|Field|Type|Description|
|-----|----|-----------|
|date_time|[pt-date-time](#pt-date-time)|A public transport date time|
|stop_point|[stop_point](#stop-point)|A stop point|

### <a name="embedded-type"></a>Embedded type

Enum used to identify what kind of objects *[/places](#places)*, *[/pt_objects](#pt-objects)* or *[/disruptions](#disruption)* services are managing.


| Value                                             | Description                                                   |
|---------------------------------------------------|---------------------------------------------------------------|
| [administrative_region](#administrative-region)   | a city, a district, a neighborhood                            |
| [network](#network)                               | a public transport network                                    |
| [commercial_mode](#commercial-mode)               | a public transport branded mode                               |
| [line](#line)                                     | a public transport line                                       |
| [route](#route)                                   | a public transport route                                      |
| [stop_area](#stop-area)                           | a nameable zone, where there are some stop points             |
| [stop_point](#stop-point)                         | a location where vehicles can pickup or drop off passengers   |
| [address](#address)                               | a point located in a street                                   |
| [poi](#poi)                                       | a point of interest                                           |
| [trip](#trip)                                     | a trip                                                        |

<aside class="notice">
    This enum is used by 3 services :<br>
    <ul>
    <li>Using <b>places</b> service, navitia would returned objects among administrative_region, stop_area, poi, address and stop_point types<br>
    <li>Using <b>pt_objects</b> service, navitia would returned objects among network, commercial_mode, stop_area, line and route types<br>
    <li>Using <b>disruptions</b> service, navitia would returned objects among network, commercial_mode, stop_area, line, route and trips types<br>
    </ul>
</aside>


### equipment

Enum from

-   has_wheelchair_accessibility
-   has_bike_accepted
-   has_air_conditioned
-   has_visual_announcement
-   has_audible_announcement
-   has_appropriate_escort
-   has_appropriate_signage
-   has_school_vehicle
-   has_wheelchair_boarding
-   has_sheltered
-   has_elevator
-   has_escalator
-   has_bike_depot

### display informations

|Field|Type|Description|
|-----|----|-----------|
|network|String|The name of the network|
|direction|String|A direction|
|commercial_mode|String|The commercial mode|
|physical_mode|String|The physical mode|
|label|String|The label of the object|
|color|String|The hexadecimal code of the line|
|code|String|The code of the line|
|description|String|A description|
|equipments|Array of String|list of [equipment](#equipment) of the object|

### link

See [interface](#interface) section.

Special Parameters
------------------

### datetime

A date time with the format YYYYMMDDThhmmss

#### depth

This tiny parameter can expand Navitia power by making it more wordy. As it is valuable on every API, take a look at [depth](#depth)


