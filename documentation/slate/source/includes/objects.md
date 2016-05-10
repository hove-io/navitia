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

Networks are fed by agencies in GTFS format.

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the network|
|name|string|Name of the network|

### Line

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the line|
|name|string|Name of the line|
|code|string|Code name of the line|
|color|string|Color of the line|
|routes|array of [route](#route)|Routes of the line|
|commercial_mode|[commercial_mode](#commercial-mode)|Commercial mode of the line|

### Route

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the route|
|name|string|Name of the route|
|is_frequence|bool|Is the route has frequency or not|
|line|[line](#line)|The line of this route|
|direction|[place](#place)|The direction of this route|

As "direction" is a [place](#place) , it can be a poi in some data.

### <a name="stop-point"></a>Stop Point

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the stop point|
|name|string|Name of the stop point|
|coord|[coord](#coord)|Coordinates of the stop point|
|administrative_regions|array of [admin](#admin)|Administrative regions of the stop point in which is the stop point|
|equipments|array of string|list of [equipment](#equipment) of the stop point|
|stop_area|[stop_area](#stop-area)|Stop Area containing this stop point|

### <a name="stop-area"></a>Stop Area

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the stop area|
|name|string|Name of the stop area|
|coord|[coord](#coord)|Coordinates of the stop area|
|administrative_regions|array of [admin](#admin)|Administrative regions of the stop area in which is the stop area|
|stop_points|array of [stop_point](#stop-point)|Stop points contained in this stop area|

### <a name="commercial-mode"></a>Commercial Mode

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the commercial mode|
|name|string|Name of the commercial mode|
|physical_modes|array of [physical_mode](#physical-mode)|Physical modes of this commercial mode|

### <a name="physical-mode"></a>Physical Mode

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

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the company|
|name|string|Name of the company|

### Place

A container containing either a [admin](#admin), [poi](#poi), [address](#address), [stop_area](#stop-area),
[stop_point](#stop-point)

|Field|Type|Description|
|-----|----|-----------|
|name|string|The name of the embedded object|
|id|string|The id of the embedded object|
|embedded_type|[embedded_type](#embedded-type)|The type of the embedded object|
|administrative_region|*optional* [admin](#admin)|Embedded administrative region|
|stop_area|*optional* [stop_area](#stop-area)|Embedded Stop area|
|poi|*optional* [poi](#poi)|Embedded poi|
|address|*optional* [address](#address)|Embedded address|
|stop_point|*optional* [stop_point](#stop-point)|Embedded Stop point|


### <a name="pt-object"></a>Pt_object

A container containing either a [network](#network), [commercial_mode](#commercial-mode), [line](#line), [route](#route),
[stop_area](#stop_point), [stop_area](#stop_point)

|Field|Type|Description|
|-----|----|-----------|
|name|string|The name of the embedded object|
|id|string|The id of the embedded object|
|embedded_type|[embedded_type](#embedded-type)|The type of the embedded object|
|stop_area|*optional* [stop_area](#stop-area)|Embedded Stop area|
|stop_point|*optional* [stop_point](#stop-point)|Embedded Stop point|
|network|*optional* [network](#network)|Embedded network|
|commercial_mode|*optional* [commercial_mode](#commercial-mode)|Embedded commercial_mode|
|stop_area|*optional* [stop_area](#stop-area)|Embedded Stop area|
|line|*optional* [line](#line)|Embedded line|
|route|*optional* [route](#route)|Embedded route|

Real time and disruption objects
--------------------------------

### Disruption

|Field | Type | Description |
|------|------|-------------|
|status | between: "past", "active" or "future" |state of the disruption
|id     | string                                |Id of the disruption
|disruption_id | string                         |for traceability: Id of original input disruption
|severity      | [severity](#severity)          |gives some categorization element
|application_periods |array of [period](#period)       |dates where the current disruption is active
|messages            |[message](#message)     |text to provide to the traveler
|updated_at          |[iso-date-time](#iso-date-time) |date_time of last modifications 
|impacted_objects    |array of [pt_object](#pt_object) |The list of public transport objects which are affected by the disruption
cause                |string                   |why is there such a disruption?

### Message

|Field|Type|Description|
|-----|----|-----------|
|text|string|a message to bring to a traveler|
|channel|[channel](#channel)|destination media. Be careful, no normalized enum for now|

### Severity

Severity object can be used to make visual grouping.

| Field         | Type         | Description                                    |
|---------------|--------------|------------------------------------------------|
| color         | string     | HTML color for classification                  |
| priority      | integer    | given by the agency : 0 is strongest priority. it can be null |
| name          | string     | name of severity                               |
| effect        | Enum       | Normalized value of the effect on the public transport object See the GTFS RT documentation at <https://developers.google.com/transit/gtfs-realtime/reference#Effect> |

### Channel

| Field         | Type       | Description
|---------------|------------|---------------------------------------------
| id            |string      |Identifier of the address
| content_type  |string      |Like text/html, you know? Otherwise, take a look at <http://www.w3.org/Protocols/rfc1341/4_Content-Type.html>
| name          |string      |name of the Channel

### Period

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


|Value|Description|
|-----|-----------|
|administrative_region|a city, a district, a neighborhood|
|network|a public transport network|
|commercial_mode|a public transport branded mode|
|line|a public transport line|
|route|a public transport route|
|stop_area|a nameable zone, where there are some stop points|
|stop_point|a location where vehicles can pickup or drop off passengers|
|address|a point located in a street|
|poi|a point of interest|

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


