APIs
====

<a name="coverage"></a>Coverage
-------------------------------

Also known as `/coverage` service.

You can easily navigate through regions covered by navitia.io, with the
coverage api. The shape of the region is provided in GeoJSON.

The only arguments are the ones of [paging](#paging).

<a name="coord"></a>Inverted geocoding
--------------------------------------

Also known as `/coord` service.

Very simple service: you give Navitia some coordinates, it answers you
where you are:

-   your detailed postal address
-   the right Navitia "coverage" which allows you to access to all known
    local mobility services

For example, you can request navitia with a WGS84 coordinate as:

<https://api.navitia.io/v1/coord/2.37691590563854;48.8467597481174>

In response, you will get the coverage id, a very useful label and a
ressource id:

``` json
{
    "regions": [
        "fr-idf"
    ],
    "address": {
        "id": "2.37691590564;48.8467597481",
        "label": "20 Rue Hector Malot (Paris)",
        "...": "..."
    }
}
```

The coverage id is *"regions": ["fr-idf"]* so you can ask Navitia on
accessible local mobility services:

<https://api.navitia.io/v1/coverage/fr-idf/>

``` json
{
    "regions": [{
        "status": "running",
        "start_production_date": "20150804","end_production_date": "20160316",
        "id": "fr-idf"
    }],
    "links": [
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/coords"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/places"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/networks"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/physical_modes"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/companies"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/commercial_modes"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/lines"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/routes"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/stop_areas"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/stop_points"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/line_groups"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/connections"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/vehicle_journeys"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/poi_types"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/pois"},
        {"href": "https://api.navitia.io/v1/coverage/fr-idf/"}
    ]
}
```

<a name="pt-ref"></a>Public transportation objects exploration
--------------------------------------------------------------

Also known as `/networks`, `/lines`, `/stop_areas`... services.


Once you have selected a region, you can explore the public
transportation objects easily with these apis. You just need to add at
the end of your url a collection name to see all the objects of a
particular collection. To see an object add the id of this object at the
end of the collection's url. The [paging](#paging) arguments may be used to
paginate results.

### Collections

-   [networks](#network)
-   [lines](#line)
-   [routes](#route)
-   [stop_points](#stop-point)
-   [stop_areas](#stop-area)
-   [commercial_modes](#commercial-mode)
-   [physical_modes](#physical-mode)
-   [companies](#company)
-   [vehicle_journeys](#vehicle-journey)
-   [disruptions](#disruption)

#### <a name="depth"></a>depth

This tiny parameter can expand Navitia power by making it more wordy.
Here is some examples around "metro line 1" from the Parisian network:

- Get "line 1" id
	- <https://api.navitia.io/v1/coverage/fr-idf/pt_objects?q=metro%201>
	- The id is "line:OIF:100110001:1OIF439"
- Get routes for this line
	- <https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:100110001:1OIF439/routes>
- Want to get a tiny response? Just add "depth=0"
	- <https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:100110001:1OIF439/routes?depth=0>
	- The response is lighter (parent lines disappear for example)
- Want more informations, just add "depth=2"
	- <https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:100110001:1OIF439/routes?depth=2>
	- The response is a little more verbose (with some geojson appear in response)
- Wanna fat more informations, let's try "depth=3"
	- <https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:100110001:1OIF439/routes?depth=3>
	- Big response: all stop_points are shown
- Wanna spam the internet bandwidth? Try "depth=42"
	- No. There is a technical limit with "depth=3"

#### odt level

-   Type: String
-   Default value: all
-   Warning: works ONLY with */[line](#line)s* collection...

It allows you to request navitia for specific pickup lines. It refers to
the [odt](#odt) section. "odt_level" can take one of these values:

-   all (default value): no filter, provide all public transport lines,
    whatever its type
-   scheduled : provide only regular lines (see the [odt](#odt) section)
-   with_stops : to get regular, "odt_with_stop_time" and "odt_with_stop_point" lines.
    -   You can easily request route_schedule and stop_schedule with these kind of lines.
    -   Be aware of "estimated" stop times
-   zonal : to get "odt_with_zone" lines with non-detailed journeys

For example

<https://api.navitia.io/v1/coverage/fr-nw/networks/network:lila/lines>

<https://api.navitia.io/v1/coverage/fr-nw/networks/network:Lignes18/lines?odt_level=scheduled>

#### distance

-   Type: Integer
-   Default value: 200

If you specify coords in your filter, you can modify the radius used for
the proximity search.
<https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/stop_schedules?distance=500>

#### headsign

-   Type: String

If given, add a filter on the vehicle journeys that has the given value
as headsign (on vehicle journey itself or at a stop time).

Examples:

-   <https://api.navitia.io/v1/coverage/fr-idf/vehicle_journeys?headsign=CANE>
-   <https://api.navitia.io/v1/coverage/fr-idf/stop_areas?headsign=CANE>


<aside class="warning">
    This last request gives the stop areas used by the vehicle
    journeys containing the headsign CANE, <b>not</b> the stop areas where it
    exists a stop time with the headsign CANE.
</aside>

#### since / until

-   Type: [iso-date-time](#iso-date-time)

To be used only on "vehicle_journeys" collection, to filter on a
period. Both parameters "until" and "since" are optional.

Example:

-   <https://api.navitia.io/v1/coverage/fr_idf/vehicle_journeys?since=20150912T120000&until=20150913T110000>

<aside class="warning">
    This filter is applied using only the first stop time of a
    vehicle journey, "since" is included and "until" is excluded.
</aside>

### Filter

It is possible to apply a filter to the returned collection, using
"filter" parameter. If no object matches the filter, a "bad_filter"
error is sent. If filter can not be parsed, an "unable_to_parse" error
is sent. If object or attribute provided is not handled, the filter is
ignored.

#### line.code

It allows you to request navitia objects referencing a line whose code
is the one provided, especially lines themselves and routes.

Examples :

-   <https://api.navitia.io/v1/coverage/fr-idf/lines?filter=line.code=4>
-   <https://api.navitia.io/v1/coverage/fr-idf/routes?filter=line.code=\"métro\ 347\">

### Few exploration examples

Response example for this request
<https://api.navitia.io/v1/coverage/fr-idf/physical_modes>

``` json
{
    "links": [
        "..."
    ],
    "pagination": {
        "..."
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
        "..."
    ]
}
```

Other examples

-   Network list
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks>
-   Physical mode list
    -   <https://api.navitia.io/v1/coverage/fr-idf/physical_modes>
-   Line list
    -   <https://api.navitia.io/v1/coverage/fr-idf/lines>
-   Line list for one mode
    -   <https://api.navitia.io/v1/coverage/fr-idf/physical_modes/physical_mode:Metro/lines>

<a name="pt-objects"></a>Autocomplete on Public Transport objects 
-----------------------------------------------------------------

Also known as `/pt_objects` service.

This api search in public transport objects via their names. It's a kind
of magical autocomplete on public transport data. It returns, in
addition of classic objects, a collection of [places](#place).

### How does it works

Differents kind of objects can be returned (sorted as):

-   network
-   commercial_mode
-   line
-   route
-   stop_area

Here is a typical use case. A traveler has to find a line between the
1500 lines around Paris. 
For example, he could key without any filters:

<div data-collapse>
    <p>traveler input: "bob"</p>
    <ul>
        <li>network : "bobby network"</li>
        <li>line : "bobby bus 1"</li>
        <li>line : "bobby bus 2"</li>
        <li>route : "bobby bus 1 to green"</li>
        <li>route : "bobby bus 1 to rose"</li>
        <li>route : "bobby bus 2 to yellow"</li>
        <li>stop_area : "...</li>
    </ul>
</div>
<div data-collapse>
    <p>traveler input: "bobby met"</p>
    <ul>
        <li>line : "bobby metro 1"</li>
        <li>line : "bobby metro 11"</li>
        <li>line : "bobby metro 2"</li>
        <li>line : "bobby metro 3"</li>
        <li>route : "bobby metro 1 to Martin"</li>
        <li>route : "bobby metro 1 to Mahatma"</li>
        <li>route : "bobby metro 11 to Marcus"</li>
        <li>route : "bobby metro 11 to Steven"</li>
        <li>route : "...</li>
    </ul>
</div>
<div data-collapse>
    <p>traveler input: "bobby met 11" or "bobby metro 11"</p>
    <ul>
        <li>line : "bobby metro 11"</li>
        <li>route : "bobby metro 11 to Marcus"</li>
        <li>route : "bobby metro 11 to Steven"</li>
    </ul>
</div>

### Parameters


  Required | Name     | Type             | Description          | Default value
-----------|----------|------------------|----------------------|-----------------
  yep      | q        | string           | The search term      |
  nop      | type[]   | array of string  | Type of objects you want to query It takes one the following values: [`network`, `commercial_mode`, `line`, `route`, `stop_area`] | [`network`, `commercial_mode`, `line`, `route`, `stop_area`]

<aside class="warning">
There is no pagination for this api
</aside>

### Example

Response example for :
<https://api.navitia.io/v1/coverage/fr-idf/pt_objects?q=bus%20ratp%20%39&type[]=line&type[]=route>

``` json
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
```

<a name="places"></a>Autocomplete on geographical objects
---------------------------------------------------------

Also known as `/places` service.

This api search in all geographical objects via their names. It returns,
in addition of classic objects, a collection of [places](#place) . This api is
very useful to make some autocomplete stuff.

Differents kind of objects can be returned (sorted as):

-   administrative_region
-   stop_area
-   poi
-   address
-   stop_point (appears only if specified, using "&type[]=stop_point"
    filter)

<aside class="warning">
    There is no pagination for this api.
</aside>

### Parameters

  Required | Name      | Type        | Description            | Default value
  ---------|-----------|-------------|------------------------|-------------------
  yep      | q           | string    | The search term        |
  nop      | type[]      | array of string | Type of objects you want to query It takes one the following values: [`stop_area`, `address`, `administrative_region`, `poi`, `stop_point`] | [`stop_area`, `address`, `poi`, `administrative_region`]
  nop      | admin_uri[] | array of string | If filled, will restrained the search within the given admin uris

### How does it works

### Example

Response example for :
<https://api.navitia.io/v1/coverage/fr-idf/places?q=rue>

``` json
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
```

<a name="places-nearby"></a>Places Nearby
-----------------------------------------

Also known as `/places_nearby` service.

This api search for public transport object near another object, or near
coordinates. It returns, in addition of classic objects, a collection of
[places](#place).

<aside class="warning">
    There is no pagination for this api.
</aside>

### Parameters

  Required | name        | Type            | Description                       | Default value
  ---------|-------------|-----------------|-----------------------------------|---------------------------------------------------------------
  nop      | distance    | int             | Distance range in meters          | 500
  nop      | type[]      | array of string | Type of objects you want to query | [`stop_area`, `stop_point`, `poi`, `administrative_region`]
  nop      | admin_uri[] | array of string | If filled, will restrained the search within the given admin uris      |
  nop      | filter      | string          |  Use to restrain returned objects. for example: places_type.id=theater |

Filters can be added:

-   request for the city of "Paris" on fr-idf
    -   <https://api.navitia.io/v1/coverage/fr-idf/places?q=paris>
-   then pois nearby this city
    -   <https://api.navitia.io/v1/coverage/fr-idf/places/admin:7444/places_nearby>
-   and then, let's catch every parking around
    -   "distance=10000" Paris is not so big
    -   "type[]=poi" to take pois only
    -   "filter=poi_type.id=poi_type:amenity:parking" to get parking
    -   <https://api.navitia.io/v1/coverage/fr-idf/places/admin:7444/places_nearby?distance=10000&count=100&type[]=poi&filter=poi_type.id=poi_type:amenity:parking>

### Example

Response example for this request
<https://api.navitia.io/v1/coverage/fr-idf/stop_areas/stop_area:TRN:SA:DUA8754575/places_nearby>

``` json
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
```

<a name="journeys"></a>Journeys
-------------------------------

Also known as `/journeys` service. This api computes journeys.

If used within the coverage api, it will retrieve the next journeys from
the selected public transport object or coordinates.

There are two ways to access this api.

The first one is: <https://api.navitia.io/v1/{a_path_to_resource}/journeys> it will
retrieve all the journeys from the resource (*isochrones*).

The other one, the most used, is to access the 'journey' api endpoint:
<<https://api.navitia.io/v1/journeys?from={resource_id_1}&to={resource_id_2}&datetime={date_time_to_leave}> .

<aside class="notice">
    Navitia.io handle lot's of different data sets (regions). Some of them
    can overlap. For example opendata data sets can overlap with private
    data sets.
    <br>
    When using the journeys endpoint the data set used to compute the
    journey is chosen using the possible datasets of the origin and the
    destination.
    <br>
    For the moment it is not yet possible to compute journeys on different
    data sets, but it will one day be possible (with a cross-data-set
    system).
</aside>

In the [examples](#examples), positions are given by coordinates and no network is specified.
However when no coordinates are provided, you need to provide on what region you want to request as
<https://api.navitia.io/v1/coverage/us-ca/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682>

The list of regions covered by navitia is available through [coverage](#coverage).


<aside class="notice">
    If you want to use a specific data set, use the journey api within the
    data set: https://api.navitia.io/v1/coverage/{your_dataset}/journeys
</aside>

<aside class="success">
    Neither the 'from' nor the 'to' parameter of the journey are required,
    but obviously one of them has to be provided.
    <br>
    If only one is defined an isochrone is computed with every possible
    journeys from or to the point.
</aside>

### <a name="journeys-parameters"></a>Main parameters

| Required  | Name                    | Type          | Description                                                                           | Default value |
|-----------|-------------------------|---------------|---------------------------------------------------------------------------------------|---------------|
| nop       | from                    | id            | The id of the departure of your journey If none are provided an isochrone is computed |               |
| nop       | to                      | id            | The id of the arrival of your journey If none are provided an isochrone is computed   |               |
| yep       | datetime                | [iso-date-time](#iso-date-time) | Date and time to go                                                                          |               |
| nop       | datetime_represents     | string        | Can be `departure` or `arrival`.<br>If `departure`, the request will retrieve journeys starting after datetime.<br>If `arrival` it will retrieve journeys arriving before datetime. | departure |
| nop       | <a name="traveler-type"></a>traveler_type           | enum          | Define speeds and accessibility values for different kind of people.<br>Each profile also automatically determines appropriate first and last section modes to the covered area. You can overload all parameters (especially speeds, distances, first and last modes) by setting all of them specifically<br><div data-collapse><p>enum values:</p><ul><li>standard</li><li>slow_walker</li><li>fast_walker</li><li>luggage</li></ul></div>| standard      |
| nop       | forbidden_uris[]        | id            | If you want to avoid lines, modes, networks, etc.                                     |               |
| nop       | data_freshness          | enum          | Define the freshness of data to use to compute journeys <ul><li>realtime</li><li>base_schedule</li></ul> _**when using the following parameter**_ "&data_freshness=base_schedule" <br> you can get disrupted journeys in the response. You can then display the disruption message to the traveler and make a realtime request to get a new undisrupted solution.                 | base_schedule |

### Other parameters

| Required | Name            | Type    | Description                   | Default value |
|----------|-----------------|---------|-------------------------------|---------------|
| nop     | first_section_mode[] | array of string   | Force the first section mode if the first section is not a public transport one. It takes one the following values: walking, car, bike, bss.<br>bss stands for bike sharing system.<br>It's an array, you can give multiple modes.<br>Note: choosing bss implicitly allows the walking mode since you might have to walk to the bss station| walking     |
| nop     | last_section_mode[]  | array of string   | Same as first_section_mode but for the last section  | walking     |
| nop     | max_duration_to_pt   | int     | Maximum allowed duration to reach the public transport.<br>Use this to limit the walking/biking part.<br>Unit is seconds | 15*60 s    |
| nop     | walking_speed        | float   | Walking speed for the fallback sections<br>Speed unit must be in meter/seconds         | 1.12 m/s<br>(4 km/h)<br>*Yes, man, they got the metric system* |
| nop     | bike_speed           | float   | Biking speed for the fallback<br>Speed unit must be in meter/seconds | 4.1 m/s<br>(14.7 km/h)   |
| nop     | bss_speed            | float   | Speed while using a bike from a bike sharing system for the fallback sections<br>Speed unit must be in meter/seconds | 4.1 m/s<br>(14.7 km/h)    |
| nop     | car_speed            | float   | Driving speed for the fallback sections<br>Speed unit must be in meter/seconds         | 16.8 m/s<br>(60 km/h)   |
| nop     | min_nb_journeys      | int     | Minimum number of different suggested journeys<br>More in multiple_journeys  |             |
| nop     | max_nb_journeys      | int     | Maximum number of different suggested journeys<br>More in multiple_journeys  |             |
| nop     | count                | int     | Fixed number of different journeys<br>More in multiple_journeys  |             |
| nop     | max_nb_tranfers      | int     | Maximum number of transfers in each journey  | 10          |
| nop     | max_duration         | int     | Maximum duration of journeys in secondes. Really useful when computing an isochrone   | 10          |
| nop     | disruption_active    | boolean | For compatibility use only.<br>If true the algorithm take the disruptions into account, and thus avoid disrupted public transport.<br>Rq: `disruption_active=true` = `data_freshness=realtime` <br>Use `data_freshness` parameter instead       |  False     |
| nop     | wheelchair           | boolean | If true the traveler is considered to be using a wheelchair, thus only accessible public transport are used<br>be warned: many data are currently too faint to provide acceptable answers with this parameter on       | False       |
| nop     | debug                | boolean | Debug mode<br>No journeys are filtered in this mode     | False       |

### Objects

Here is a typical journey, all sections are detailed below

![image](typical_itinerary.png)

#### Main response

|Field|Type|Description|
|-----|----|-----------|
|journeys|array of [journeys](#journey)|List of computed journeys|
|links|[link](#link)|Links related to the journeys|

#### Journey

  Field               | Type                         | Description
  --------------------|------------------------------|-----------------------------------
  duration            | int                          | Duration of the journey
  nb_transfers        | int                          | Number of transfers in the journey
  departure_date_time | [iso-date-time](#iso-date-time) | Departure date and time of the journey
  requested_date_time | [iso-date-time](#iso-date-time) | Requested date and time of the journey
  arrival_date_time   | [iso-date-time](#iso-date-time) | Arrival date and time of the journey
  sections            | array of [section](#section) |  All the sections of the journey
  from                | [places](#place)            | The place from where the journey starts
  to                  | [places](#place)            | The place from where the journey ends
  links               | [link](#link)                | Links related to this journey
  type                | *enum* string                | Used to qualify a journey. See the [journey-qualif](#journey-qualif) section for more information
  fare                | [fare](#fare)                | Fare of the journey (tickets and price)
  tags                | array of string              | List of tags on the journey. The tags add additional information on the journey beside the journey type. See for example [multiple_journeys](#multiple-journeys).
  status              | *enum*                       | Status from the whole journey taking into acount the most disturbing information retrieved on every object used. Can be: <ul><li>NO_SERVICE</li><li>REDUCED_SERVICE</li><li>SIGNIFICANT_DELAYS</li><li>DETOUR</li><li>ADDITIONAL_SERVICE</li><li>MODIFIED_SERVICE</li><li>OTHER_EFFECT</li><li>UNKNOWN_EFFECT</li><li>STOP_MOVED</li></ul> In order to get a undisrupted journey, you just have to add a *&data_freshness=realtime* parameter

<aside class="notice">
    When used with just a "from" or a "to" parameter, it will not contain any sections.
</aside>

#### Section

Field                    | Type                                          | Description
-------------------------|-----------------------------------------------|------------
type                     | *enum* string                                 | <div data-collapse><p>Type of the section.</p><ul><li>`public_transport`: public transport section</li><li>`street_network`: street section</li><li>`waiting`: waiting section between transport</li><li><p>`stay_in`: this “stay in the vehicle” section occurs when the traveller has to stay in the vehicle when the bus change its routing. Here is an exemple for a journey from A to B: (lollipop line)</p><p>![image](stay_in.png)</p></li><li>`transfer`: transfert section</li><li><p>`crow_fly`: teleportation section. Used when starting or arriving to a city or a stoparea (“potato shaped” objects) Useful to make navitia idempotent. Be careful: no “path” nor “geojson” items in this case</p><p>![image](crow_fly.png)</p></li><li>`on_demand_transport`: vehicle may not drive along: traveler will have to call agency to confirm journey</li><li>`bss_rent`: taking a bike from a bike sharing system (bss)</li><li>`bss_put_back`: putting back a bike from a bike sharing system (bss)</li><li>`boarding`: boarding on plane</li><li>`landing`: landing off the plane</li></ul></div>
id                       | string                                        | Id of the section
mode                     | *enum* string                                 | Mode of the street network: `Walking`, `Bike`, `Car`
duration                 | int                                           | Duration of this section
from                     | [places](#place)                             | Origin place of this section
to                       | [places](#place)                             | Destination place of this section
links                    | Array of [link](#link)                        | Links related to this section
display_informations     | [display_informations](#display-informations) | Useful information to display
additionnal_informations | *enum* string                                 | Other information. It can be: <ul><li>`regular`: no on demand transport (odt)</li><li>`has_date_time_estimated`: section with at least one estimated date time</li><li>`odt_with_stop_time`: odt with fixed schedule, but travelers have to call agency!</li><li>`odt_with_stop_point`: odt where pickup or drop off are specific points</li><li>`odt_with_zone`: odt which is like a cab, from wherever you want to wherever you want, whenever it is possible</li></ul>
geojson                  | [GeoJson](http://www.geojson.org)             |
path                     | Array of [path](#path)                        | The path of this section
transfer_type            | *enum* string                                 | The type of this transfer it can be: `walking`, `guaranteed`, `extension`
stop_date_times          | Array of [stop_date_time](#stop_date_time)    | List of the stop times of this section
departure_date_time      | [iso-date-time](#iso-date-time)               | Date and time of departure
arrival_date_time        | [iso-date-time](#iso-date-time)               | Date and time of arrival

#### Path

A path object in composed of an array of [path_item](#path-item) (segment).

#### Path item


Field           | Type                   | Description
----------------|------------------------|------------
length          | int                    | Length (in meter) of the segment
name            | string                 | name of the way corresponding to the segment
duration        | int                    | duration (in seconds) of the segment
direction       | int                    | Angle (in degree) between the previous segment and this segment.<br><ul><li>0 means going straight</li><li>> 0 means turning right</li><li>< 0 means turning left</li></ul><br>Hope it's easier to understand with a picture: ![image](direction.png)

#### Fare

|Field|Type|Description|
|-----|----|-----------|
|total|[cost](#cost) |total cost of the journey|
|found|boolean|False if no fare has been found for the journey, True otherwise|
|links|[link](#link) |Links related to this object. Link with related [tickets](#ticket)|

#### Cost

|Field|Type|Description|
|-----|----|-----------|
|value|float|cost|
|currency|string|currency|

#### Ticket

|Field|Type|Description|
|-----|----|-----------|
|id|string|Id of the ticket|
|name|string|Name of the ticket|
|found|boolean|False if unknown ticket, True otherwise|
|cost|[cost](#cost)|Cost of the ticket|
|links|array of [link](#link)|Link to the [section](#section) using this ticket|

<a name="route-schedules"></a>Route Schedules and time tables
-------------------------------------------------------------

Also known as `/route_schedules` service.

This api gives you access to schedules of routes. The response is made
of an array of route_schedule, and another one of [note](#note). You can
access it via that kind of url: <https://api.navitia.io/v1/{a_path_to_a_resource}/route_schedules>

### Parameters

Required | Name               | Type      | Description                                                                              | Default Value
---------|--------------------|-----------|------------------------------------------------------------------------------------------|--------------
yep      | from_datetime      | [iso-date-time](#iso-date-time) | The date_time from which you want the schedules                    |
nop      | duration           | int       | Maximum duration in seconds between from_datetime and the retrieved datetimes.           | 86400
nop      | items_per_schedule | int       | Maximum number of columns per schedule.                                                  |
nop      | forbidden_uris[]   | id        | If you want to avoid lines, modes, networks, etc.                                        | 
nop      | data_freshness     | enum      | Define the freshness of data to use<br><ul><li>realtime</li><li>base_schedule</li></ul>  | base_schedule


### Objects

#### route_schedule object

|Field|Type|Description|
|-----|----|-----------|
|display_informations|[display_informations](#display-informations)|Usefull information about the route to display|
|Table|[table](#table)|The schedule table|

#### table

|Field|Type|Description|
|-----|----|-----------|
|Headers|Array of [header](#header)|Informations about vehicle journeys|
|Rows|Array of [row](#row)|A row of the schedule|

#### header

Field                    | Type                                          | Description
-------------------------|-----------------------------------------------|---------------------------
additionnal_informations | Array of String                               | Other information: TODO enum
display_informations     | [display_informations](#display-informations) | Usefull information about the the vehicle journey to display
links                    | Array of [link](#link)                        | Links to [line](#line), vehicle_journey, [route](#route), [commercial_mode](#commercial-mode), [physical_mode](#physical-mode), [network](#network)

#### row

Field      | Type                             | Description
-----------|----------------------------------|--------------------------
date_times | Array of [pt-date-time](#pt-date-time) | Array of public transport formated date time
stop_point | [stop_point](#stop-point)              | The stop point of the row

<a name="stop-schedules"></a>Stop Schedules and other kind of time tables
-------------------------------------------------------------------------

Also known as `/stop_schedules` service.

This api gives you access to schedules of stops going through a stop
point. The response is made of an array of stop_schedule, and another
one of [note](#note). You can access it via that kind of url: <https://api.navitia.io/v1/{a_path_to_a_resource}/stop_schedules>

### Parameters

Required | Name           | Type                    | Description        | Default Value
---------|----------------|-------------------------|--------------------|--------------
yep      | from_datetime  | [iso-date-time](#iso-date-time) | The date_time from which you want the schedules |
nop      | duration         | int                            | Maximum duration in seconds between from_datetime and the retrieved datetimes.                            | 86400
nop      | forbidden_uris[] | id                             | If you want to avoid lines, modes, networks, etc.    | 
nop      | items_per_schedule | int       | Maximum number of datetimes per schedule.                                                  |
nop      | data_freshness   | enum                           | Define the freshness of data to use to compute journeys <ul><li>realtime</li><li>base_schedule</li></ul> | base_schedule


### Stop_schedule object

|Field|Type|Description|
|-----|----|-----------|
|display_informations|[display_informations](#display-informations)|Usefull information about the route to display|
|route|[route](#route)|The route of the schedule|
|date_times|Array of [pt-date-time](#pt-date-time)|When does a bus stops at the stop point|
|stop_point|[stop_point](#stop-point)|The stop point of the schedule|

<a name="departures"></a>Departures
-----------------------------------

Also known as `/departures` service.

This api retrieves a list of departures from a datetime of a selected
object. Departures are ordered chronologically in ascending order.

### Parameters

Required | Name           | Type                    | Description        | Default Value
---------|----------------|-------------------------|--------------------|--------------
yep      | from_datetime    | [iso-date-time](#iso-date-time) | The date_time from which you want the schedules |
nop      | duration         | int                             | Maximum duration in seconds between from_datetime and the retrieved datetimes.                            | 86400
nop      | forbidden_uris[] | id                              | If you want to avoid lines, modes, networks, etc.    | 
nop      | data_freshness   | enum                            | Define the freshness of data to use to compute journeys <ul><li>realtime</li><li>base_schedule</li></ul> | realtime


### Departure objects

|Field|Type|Description|
|-----|----|-----------|
|route|[route](#route)|The route of the schedule|
|stop_date_time|Array of [stop_date_time](#stop_date_time)|When does a bus stops at the stop point|
|stop_point|[stop_point](#stop-point)|The stop point of the schedule|

<a name="arrivals"></a>Arrivals
-------------------------------

Also known as `/arrivals` service.

This api retrieves a list of arrivals from a datetime of a selected
object. Arrivals are ordered chronologically in ascending order.

<a name="traffic-reports"></a>Traffic reports
---------------------------------------------

Also known as `/traffic_reports` service.

This service provides the state of public transport traffic. It can be
called for an overall coverage or for a specific object.

### Parameters

You can access it via that kind of url: <https://api.navitia.io/v1/{a_path_to_a_resource}/traffic_reports>

For example:

-   overall public transport traffic report on Ile de France coverage
    -   <https://api.navitia.io/v1/coverage/fr-idf/traffic_reports>
-   Is there any perturbations on the RER network ?
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/traffic_reports>
-   Is there any perturbations on the "RER A" line ?
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/lines/line:TRN:DUA810801043/traffic_reports>

The response is made of an array of [traffic_reports](#traffic-reports), and another one
of [disruptions](#disruption). There are inner links between this 2 arrays: see the
[inner-reference](#inner-reference) section to use them.

### Objects

#### Traffic report

Traffic_reports is an array of some traffic_report object. One
traffic_report object is a complex object, made of a network, an array
of lines and an array of stop_areas. A typical traffic_report object
will contain:

-   1 network which is the grouping object
    -   it can contain links to its disruptions. These disruptions are globals and might not be applied on lines or stop_areas.
-   0..n lines
    -   each line contains at least a link to its disruptions
-   0..n stop_areas
    -   each stop_area contains at least a link to its disruptions

It means that if a stop_area is used by many networks, it will appear
many times.

Here is a typical response

``` json
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

```

This typical response means:

-   traffic_reports
    -   network "bob"
          -   line "1" > internal link to disruption "green"
          -   line "12" > internal link to disruption "pink"
          -   stop_area "bobito" > internal link to disruption "red"
    -   network "bobette" > internal link to disruption "blue"
          -   line "A" > internal link to disruption "green"
          -   line "C" > internal link to disruption "yellow"
          -   stop_area "bobito" > internal link to disruption "red"
-   disruptions (disruption target links)
    -   disruption "green"
    -   disruption "pink"
    -   disruption "red"
    -   disruption "blue"
    -   disruption "yellow"
    -   Each disruption contains the messages to show.

Details for disruption objects : [disruptions](#disruptions)


