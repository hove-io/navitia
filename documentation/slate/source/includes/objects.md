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

Navitia

-   exposes every date times as local times of the coverage via an ISO 8601 "YYYYMMDDThhmmss" string
-   can be requested using local times of the coverage via ISO 8601 as "YYYYMMDDThhmmss" or "YYYY-MM-DDThh:mm:ss"
-   can be requested using UTC relative times via ISO 8601 as "YYYYMMDDThhmmss+HHMM" or "YYYY-MM-DDThh:mm:ss+HH:MM"
-   can be requested using UTC times via ISO 8601 as "YYYYMMDDThhmmssZ" or "YYYY-MM-DDThh:mm:ssZ"

[Context](#context) object provides the Timezone, useful to interpret datetimes of the response.

For example:

- <https://api.navitia.io/v1/journeys?from=bob&to=bobette&datetime=20140425T1337>
- <https://api.navitia.io/v1/journeys?from=bob&to=bobette&datetime=2014-04-25T13:37+02:00>
- <https://api.navitia.io/v1/journeys?from=bob&to=bobette&datetime=2014-04-25T13:37:42Z>

There are lots of ISO 8601 libraries in every kind of language that you should use before breaking down <https://youtu.be/-5wpm-gesOY>

### Iso-date

The date are represented in ISO 8601 "YYYYMMDD" string.

Public transport objects
------------------------

### <a name="network"></a>Network

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

### <a name="line"></a>Line

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
|physical_modes|array of [physical_mode](#physical-mode)|Physical modes of the line|

### <a name="route"></a>Route

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
|is_frequence|enum|If the route has frequency or not. Can only be "False", but may be "True" in the future|
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
|label|string|Label of the stop area. The name is directly taken from the data whereas the label is something we compute for better traveler information. If you don't know what to display, display the label.|
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

Commercial modes are close from physical modes, but not normalized and can refer to a brand,
something that can be specific to a network, and known to the traveler.
Examples: RER in Paris, Busway in Nantes, and also of course Bus, Métro, etc.

Integrators should mainly use that value for text output to the traveler.

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

Physical modes are fastened and normalized (though the list can -rarely- be extended).
So it's easier for integrators to map it to a pictogram, but prefer [commercial_mode](#commercial-mode) for a text output.

The idea is to use physical modes when building a request to Navitia,
and commercial modes when building an output to the traveler.

Example: If you want to propose modes filter in your application, you should use [physical_mode](#physical-mode) rather than
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
-   physical_mode:RailShuttle
-   physical_mode:RapidTransit
-   physical_mode:Shuttle
-   physical_mode:SuspendedCableCar
-   physical_mode:Taxi
-   physical_mode:Train
-   physical_mode:Tramway

You can use these ids in the forbidden_uris[] parameter from
[journeys parameters](#journeys-parameters) for example.

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

### <a name="place"></a>Place

A container containing either a [admin](#admin), [poi](#poi), [address](#address), [stop_area](#stop-area) or
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
|quality|integer|The quality of the place|
|embedded_type|[place embedded_type](#place-embedded-type)|The type of the embedded object|
|administrative_region|[admin](#admin)|Embedded administrative region|
|stop_area|[stop_area](#stop-area)|Embedded Stop area|
|poi|[poi](#poi)|Embedded poi|
|address|[address](#address)|Embedded address|
|stop_point|[stop_point](#stop-point)|Embedded Stop point|

### <a name="trip"></a>Trip

A trip corresponds to a scheduled vehicle circulation (and all its linked real-time and disrupted routes).

Example: a train, routing a Paris to Lyon itinerary every day at 06h29, is the "Trip" named "6641".

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

It encapsulates many instances of vehicle_journey.

### <a name="vehicle-journey"></a>Vehicle-journey

A vehicle-journey describes a scheduled vehicle circulation, the days on which it circulates according to base-schedule,
the days it circulates according to realtime information.

Note that multiple vehicle-journeys are often associated with the same trip for technical and logical
reasons (to model the daylight saving time, for the changes after applying realtime disruptions, etc.).

The collection is accessible with the url:

`https://api.navitia.io/v1/coverage/sandbox/trips/{trip.id}/vehicle_journeys`.

Note: this collection is mostly used for debug and technical purposes.

### <a name="pt-object"></a>Pt_object

A container containing either a [network](#network), [commercial_mode](#commercial-mode), [line](#line), [route](#route),
[stop_point](#stop-point), [stop_area](#stop-area), [trip](#trip)

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
|quality|integer|The quality of the object|
|embedded_type|[pt-object embedded_type](#pt-object-embedded-type)|The type of the embedded object|
|stop_area|[stop_area](#stop-area)|Embedded Stop area|
|stop_point|[stop_point](#stop-point)|Embedded Stop point|
|network|[network](#network)|Embedded network|
|commercial_mode|[commercial_mode](#commercial-mode)|Embedded commercial_mode|
|stop_area|[stop_area](#stop-area)|Embedded Stop area|
|line|[line](#line)|Embedded line|
|route|[route](#route)|Embedded route|
|trip|[trip](#trip)|Embedded trip|

Real time and disruption objects
--------------------------------

### <a name="disruption"></a>Disruption

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
    "cause": "Cause...",
    "category": "incident"
}
```

|Field | Type | Description |
|------|------|-------------|
|id     | string                                |Id of the disruption
|status | between: "past", "active" or "future" |state of the disruption. The state is computed using the application_periods of the disruption and the current time of the query.
|disruption_id | string                         |for traceability: Id of original input disruption
|impact_id     | string                         |for traceability: Id of original input impact
|severity      | [severity](#severity)          |gives some categorization element
|application_periods |array of [period](#period)      |dates where the current disruption is active
|messages            |array of [message](#message)    |texts to provide to the traveler
|updated_at          |[iso-date-time](#iso-date-time) |date_time of last modifications
|impacted_objects    |array of [impacted_object](#impacted-object) |The list of public transport objects which are affected by the disruption
|cause               |string                   |why is there such a disruption?
|category            |string                   |The category of the disruption, such as "construction works" or "incident"
|contributor         |string                   |The source from which Navitia received the disruption
|uri                 |string                   |deprecated
|disruption_uri      |string                   |deprecated

### <a name="impacted-object"></a>Impacted_object

``` json
{
    "pt_object": {
        "id": "id_of_the_line",
        "name": "name of a lne",
        "embedded_type": "line",
        "line": {
            "...": "..."
        }
    },
    "impacted_section": {
        "...": "..."
    }
}
```

|Field|Type|Description|
|-----|----|-----------|
|pt_object|[pt_object](#pt-object)|The impacted public transport object|
|impacted_section|[impacted_section](#impacted-section)|Only for line section impact, the impacted section|
|impacted_stops|array of [impacted_stop](#impacted-stop)|Only for [trip](#trip) delay, the list of delays, stop by stop

### <a name="impacted-section"></a>Impacted_section

``` json
{
    "from": {
        "embedded_type": "stop_area",
        "id": "C",
        "name": "C",
        "stop_area": {
            "...": "..."
        }
    },
    "to": {
        "embedded_type": "stop_area",
        "id": "E",
        "name": "E",
        "stop_area": {
            "...": "..."
        }
    }
}
```

|Field|Type|Description|
|-----|----|-----------|
|from|[pt_object](#pt-object)|The beginning of the section|
|to|[pt_object](#pt-object)|The end of the section. This can be the same as `from` when only one point is impacted|
|routes|[route](#route)| The list of impacted routes by the impacted_section|

### <a name="impacted-stop"></a>Impacted_stop

```json
{
    "stop_point": {
        "...": "..."
    },
    "amended_departure_time": "073600",
    "base_arrival_time": "073600",
    "base_departure_time": "073600",
    "cause": "",
    "stop_time_effect": "delayed",
    "departure_status": "delayed",
    "arrival_status": "deleted"
}
```

|Field|Type|Description|
|-----|----|-----------|
|stop_point|[stop_point](#stop-point)|The impacted stop point of the trip|
|amended_departure_time|string|New departure hour (format HHMMSS) of the trip on this stop point|
|amended_arrival_time|string|New arrival hour (format HHMMSS) of the trip on this stop point|
|base_departure_time|string|Base departure hour (format HHMMSS) of the trip on this stop point|
|base_arrival_time|string|Base arrival hour (format HHMMSS) of the trip on this stop point|
|cause|string|Cause of the modification|
|stop_time_effect|Enum|Can be: "added", "deleted", "delayed" or "unchanged". *Deprecated*, consider the more accurate departure_status and arrival_status|
|arrival_status|Enum|Can be: "added", "deleted", "delayed" or "unchanged".|
|departure_status|Enum|Can be: "added", "deleted", "delayed" or "unchanged".|

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
| priority      | integer    | given by the agency: 0 is strongest priority. it can be null |
| name          | string     | name of severity                               |
| effect        | Enum       | Normalized value of the effect on the public transport object. See the GTFS RT documentation at <https://gtfs.org/reference/realtime/v2/#enum-effect>. See also [realtime](#realtime) section. |

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

### Stands

A description of the number of stands/places and vehicles available at a bike sharing station.

|Field           |Type|Description                                                                  |
|----------------|----|-----------------------------------------------------------------------------|
|available_places|int |Number of places where one can park                                          |
|available_bikes |int |Number of bikes available                                                    |
|total_stands    |int |Total number of stands (occupied or not, with or without special equipment)  |
|status          |enum|Information about the station itself:<ul><li>`unavailable`: Navitia is not able to obtain information about the station</li><li>`open`: The station is open</li><li>`closed`: The station is closed</li></ul>|

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

|Field   |Type                 |Description                                                         |
|--------|---------------------|--------------------------------------------------------------------|
|id      |string               |Identifier of the poi                                               |
|name    |string               |Name of the poi                                                     |
|label   |string               |Label of the poi. The name is directly taken from the data whereas the label is something we compute for better traveler information. If you don't know what to display, display the label.|
|poi_type|[poi_type](#poi-type)|Type of the poi                                                     |
|stands  |[stands](#stands)    |Information on the spots available, for BSS stations                |

### <a name="access-point"></a>Access_point

``` shell
$ curl 'https://api.navitia.io/v1/coverage/sandbox/access_points' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'
```

Access_point = point of access from the pavement to a station, a multimodal area.

|Field                  |Type                     |Description                                                         |
|-----------------------|-------------------------|--------------------------------------------------------------------|
|id                     |string                   |Identifier of the access point                                      |
|name                   |string                   |Name of the access point                                            |
|coord                  |[coord](#coord)          |Coordinates of the access point                                     |
|access_point_code      |string                   |Identifies the well-known code for the access_point                 |

You should labelized the access-point using "access_point_code" and "name".
For example: "follow the `access_point_code` - `name` to exit from `parent_station` "

### <a name="pathway"></a>Pathway

Pathway = indoor way from a stop point to an access point. It could be an entrance, an exit or both.

|Field                  |Type                     |Description                                                         |
|-----------------------|-------------------------|--------------------------------------------------------------------|
|id                     |string                   |Identifier of the access point                                      |
|name                   |string                   |Name of the access point                                            |
|is_entrance            |boolean                  |Identifies whether the path is an entrance                          |
|is_exit                |boolean                  |Identifies whether the path is an exit                              |
|length                 |int                      |Length of the path                                                  |
|traversal_time         |int                      |Duration to walk the path when it's known                           |

You can find pathways in [journeys](#journeys) service, where there is some "vias" in walking sections. "Vias" are "pathways" in navitia.

### Address

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the address|
|name|string|Name of the address|
|label|string|Label of the address. The name is directly taken from the data whereas the label is something we compute for better traveler information. If you don't know what to display, display the label.|
|coord|[coord](#coord)|Coordinates of the address|
|house_number|int|House number of the address|
|administrative_regions|array of [admin](#admin)|Administrative regions of the address in which is the stop area|

### <a name="admin"></a>Administrative region

|Field|Type|Description|
|-----|----|-----------|
|id|string|Identifier of the address|
|name|string|Name of the address|
|label|string|Label of the administrative region. The name is directly taken from the data whereas the label is something we compute for better traveler information. If you don't know what to display, display the label.|
|coord|[coord](#coord)|Coordinates of the address|
|level|int|Level of the admin|
|zip_code|string|Zip code of the admin|

Cities are mainly on the 8 level, dependant on the country
(<http://wiki.openstreetmap.org/wiki/Tag:boundary%3Dadministrative>)


### <a name="equipment-reports"></a>Equipment reports

```json
"equipment_reports": [
    {
        "line": {...},
        "stop_area_equipments": [...]
    },
    ...
]
```

A list of objects that maps each line with its associated stop area equipments.

|Field|Type|Description|
|-----|----|-----------|
|line|[line](#line)|The line to which equipments are associated |
|stop_area_equipments|[stop area equipments](#stop-area-equipments)|A list of objects that describes equipments for each stop area |

### <a name="stop-area-equipments"></a>Stop area equipments

```json
"stop_area_equipments": [
    {
        "equipment_details": [...],
        "stop_area": {...},
    },
    ...
]
```
A list of objects that maps equipments details for each stop area.

|Field|Type|Description|
|-----|----|-----------|
|equipment_details|[Equipment details](#equipment-details)|The equipment details associated with the stop area|
|stop_area|[Stop Area](#stop-area)|The stop area to which the `equipment_details` is associated |

### <a name="equipment-details"></a>Equipment details
```json
"equipment_details": [
    {
        "current_availability": {...},
        "embedded_type": "escalator",
        "id": "2702",
        "name": "Escalator 2702, for platform 3",
    },
    ...
]
```

|Field|Type|Occurrence|Description|
|-----|----|--------|-----------|
current_availability|[Equipment availability](#equipment-availability)|always| Describes equipments information like: status, name, id etc...
embedded_type|string|always|Define the equipment type: `escalator`, `elevator`
id|string|always|The equipment's unique identifier
name|string|optional|the equipment's name/description

### <a name="equipment-availability"></a>Equipment availability

```json
"current_availability": {
    "cause": {
        "label": "engineering work in progress"
    },
    "effect": {
        "label": "platform 3 available via stairs only"
    },
    "periods": [
        {
            "begin": "20190216T000000",
            "end": "20190601T220000"
        }
    ],
    "status": "unavailable",
}
```

|Field|Type|Required|Description|
|-----|----|--------|-----------|
status|string|always|Equipment status: <ul><li>`unknown`: no realtime information available</li><li>`unavailable`: equipment is known to be unavailable with details provided below </li></ul>
cause|label|optional|If status is `unavailable`, gives you the cause in a label
effect|label|optional|If status is `unavailable`, gives you the effect in a label
periods|period|optional|If status is `unavailable`, gives the affected period (with a `begin` & `end` datetime attributes)

Other objects
-------------

### <a name="context"></a>context

``` shell
#links between objects in a traffic_reports response
{
    "context": {
        "timezone": "Europe\/Paris",
        "current_datetime": "20171201T120114",
        "car_direct_path": {
            "co2_emission": {
                "value": 857.951371579,
                "unit": "gEC"
            }
        }
    },
    "journeys": [ ... ]
}
```

context object is a complex object provided in any endpoint's response.
It serves several goals:

-   `timezone` provides timezone of any datetime in the response.
-   `current_datetime` provides the time the call was made.<br>It is precious to compute the waiting time until next passages (journeys, departures, etc.), as when no datetime is provided at call, Navitia uses that "current_datetime" as reference time.
-   `car_direct_path` can also be provided in journeys to help compare ecological footprint of transport.

### <a name="pt-date-time"></a>pt-date-time

pt-date-time (pt stands for "public transport") is a complex date time object to manage the difference between stop and leaving times at a stop.

|Field                    | Type               | Description
|-------------------------|--------------------|-----------------------------
|additional_informations  | Array of String    | Other information: TODO enum
|departure_date_time      | [iso-date-time](#iso-date-time)    | A date time
|arrival_date_time        | [iso-date-time](#iso-date-time)    | A date time
|links                    | Array of [link](#link) |internal links to notes

### note

|Field|Type|Description|
|-----|----|-----------|
|id|String|id of the note|
|value|String|The content of the note|

### <a name="stop-date-time"></a>stop_date_time

|Field|Type|Description|
|-----|----|-----------|
|date_time|[pt-date-time](#pt-date-time)|A public transport date time|
|stop_point|[stop_point](#stop-point)|A stop point|

### <a name="place-embedded-type"></a>Place embedded type

Enum used to identify what kind of objects *[/places](#places)* and *[/places_nearby](#places-nearby-api)* services are managing.
It's also used inside different responses (journeys, ...).

| Value                                             | Description                                                   |
|---------------------------------------------------|---------------------------------------------------------------|
| [administrative_region](#administrative-region)   | a city, a district, a neighborhood                            |
| [stop_area](#stop-area)                           | a nameable zone, where there are some stop points             |
| [stop_point](#stop-point)                         | a location where vehicles can pickup or drop off passengers   |
| [address](#address)                               | a point located in a street                                   |
| [poi](#poi)                                       | a point of interest                                           |


### <a name="pt-object-embedded-type"></a>PT-object embedded type

Enum used to identify what kind of objects *[/pt_objects](#pt-objects)* service is managing.
It's also used inside different responses (disruptions, ...).

| Value                                             | Description                                                   |
|---------------------------------------------------|---------------------------------------------------------------|
| [network](#network)                               | a public transport network                                    |
| [commercial_mode](#commercial-mode)               | a public transport branded mode                               |
| [line](#line)                                     | a public transport line                                       |
| [route](#route)                                   | a public transport route                                      |
| [stop_area](#stop-area)                           | a nameable zone, where there are some stop points             |
| [stop_point](#stop-point)                         | a location where vehicles can pickup or drop off passengers   |
| [trip](#trip)                                     | a trip                                                        |

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
|physical_mode|String|The [physical_mode](#physical-mode). Physical mode are standardized|
|commercial_mode|String|The [commercial_mode](#commercial-mode). Commercial mode are not standardized|
|code|String|The code of the line|
|color|String|Hexadecimal color code for the line logo|
|text_color|String|Hexadecimal color code of the text for the line logo|
|direction|String|Direction of the trip used in a "journey" section|
|headsign|String|Text that appears on vehicle signage identifying the trip's destination for example|
|label|String|The label of the object|
|name|String|Full name of the line|
|trip_short_name|String|Short name of the route|
|equipments|Array of String|list of [equipment](#equipment) of the object|
|description|String|An optionnal description|

### link

See [interface](#interface) section.

Special Parameters
------------------

### datetime

A date time with the format YYYYMMDDThhmmss, considered local to the coverage being used.

#### depth

This tiny parameter can expand Navitia power by making it more wordy. As it is valuable on every API, take a look at [depth](#depth)
