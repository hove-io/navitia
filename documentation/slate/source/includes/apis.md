API catalog
===========

<a name="coverage"></a>Coverage
-------------------------------

Also known as `/coverage` service.

You can easily navigate through regions covered by navitia.io, with the
coverage api. The shape of the region is provided in GeoJSON.

The only arguments are the ones of [paging](#paging).

### Accesses

| url | Result |
|----------------------------------------------|-------------------------------------|
| `coverage`                              | List of the areas covered by navitia|
| `coverage/{region_id}`                  | Information about a specific region |
| `coverage/{region_id}/coords/{lon;lat}` | Information about a specific region |


<a name="coord"></a>Inverted geocoding
--------------------------------------
``` shell
#request 
$ curl 'https://api.navitia.io/v1/coords/2.37705;48.84675' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response where you can find the right Navitia coverage, and a useful label
HTTP/1.1 200 OK

{
    "regions": [
        "sandbox"
    ],
    "address": {
        "id": "2.37705;48.84675",
        "label": "20 Rue Hector Malot (Paris)",
        "...": "..."
    }
}
```

> in this example, the coverage id is "regions": ["sandbox"] 
so you can ask Navitia on accessible local mobility services:

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 200 OK

{
    "regions": [{
        "status": "running",
        "start_production_date": "20160101","end_production_date": "20160831",
        "id": "sandbox"
    }],
    "links": [
        {"href": "https://api.navitia.io/v1/coverage/sandbox/coords"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/places"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/networks"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/physical_modes"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/companies"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/commercial_modes"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/lines"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/routes"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/stop_areas"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/stop_points"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/line_groups"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/connections"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/vehicle_journeys"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/poi_types"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/pois"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/disruptions"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/datasets"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/line_groups"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/trips"},
        {"href": "https://api.navitia.io/v1/coverage/sandbox/"}
    ]
}

```

Also known as `/coords` service.

Very simple service: you give Navitia some coordinates, it answers you

-   your detailed postal address
-   the right Navitia "coverage" which allows you to access to all known
    local mobility services

You can also combine `/coords` with other filter as : 

-   get [POIs](#poi) near a coordinate
	- <https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/pois?distance=1000>
-   get specific POIs near a coordinate
	- <https://api.navitia.io/v1/coverage/fr-idf/poi_types/poi_type:amenity:bicycle_rental/coords/2.377310;48.847002/pois?distance=1000>



<a name="pt-ref"></a>Public Transportation Objects exploration
--------------------------------------------------------------

Also known as `/networks`, `/lines`, `/stop_areas`... services.


Once you have selected a region, you can explore the public
transportation objects easily with these APIs. You just need to add at
the end of your URL a collection name to see every objects within a
particular collection. To see an object detail, add the id of this object at the
end of the collection's URL. The [paging](#paging) arguments may be used to
paginate results.

### Accesses

| url | Result |
|---------------------------------------------------------|-------------------------------------|
| `/coverage/{region_id}/{collection_name}`               | Collection of objects of a region   |
| `/coverage/{region_id}/{collection_name}/{object_id}`   | Information about a specific region |
| `/coverage/{lon;lat}/{collection_name}`                 | Collection of objects of a region   |
| `/coverage/{lon;lat}/{collection_name}/{object_id}`     | Information about a specific region |

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

### Parameters

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

-   Getting every active New Jersey vehicles between 12h00 and 12h01, on a specific date <https://api.navitia.io/v1/coverage/us-ny/networks/network:newjersey/vehicle_journeys?since=20160503T120000&until=20160503T120100>

<aside class="warning">
    This filter is applied using only the first stop time of a
    vehicle journey, "since" is included and "until" is excluded.
</aside>

### <a name="filter"></a>Filter

It is possible to apply a filter to the returned collection, using
"filter" parameter. If no object matches the filter, a "bad_filter"
error is sent. If filter can not be parsed, an "unable_to_parse" error
is sent. If object or attribute provided is not handled, the filter is
ignored.

#### {collection}.has_code
``` shell
#for any pt_object request, as this one:
$ curl 'https://api.navitia.io/v1/coverage/sandbox/stop_areas' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#you can find codes on every pt_object, like:
HTTP/1.1 200 OK

{
    "stop_areas": [
        {
            "codes" :[
                {
                    "type": "external_code",
                    "value": "RATCAMPO"
                },
                {
                    "type" : "source",
                    "value" : "CAMPO"
                }
            ]
            "...": "...",
        },
        {...}
]

#you can request for objects with a specific code 
#for example, you can get this stoparea, having a "source" code "CAMPO"
#by using parameter "filter=stop_area.has_code(source,CAMPO)" like:

$ curl 'https://api.navitia.io/v1/coverage/sandbox/stop_areas?filter=stop_area.has_code(source,CAMPO)' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'
```

Every object managed by Navitia comes with its own list of ids. 
You will find some source ids, merge ids, etc. in "codes" list in json responses.
Be careful, these codes may not be unique. The navitia id is the only unique id.

You may have to request an object by one of these ids, in order to call an external service for example.

The filter format is `filter={collection_name}.has_code({code_type},{code_value})`

Examples :

-   <https://api.navitia.io/v1/coverage/fr-sw/stop_points?filter=stop_point.has_code(source,5852)>
-   <https://api.navitia.io/v1/coverage/fr-sw/stop_areas?filter=stop_area.has_code(gtfs_stop_code,1)>
-   <https://api.navitia.io/v1/coverage/fr-sw/lines?filter=line.has_code(source,11821949021891615)>

<aside class="warning">
    these ids (which are not Navitia ids) may not be unique. you will have to manage a tuple in response.
</aside>


#### line.code

It allows you to request navitia objects referencing a line whose code
is the one provided, especially lines themselves and routes.

Examples :

-   <https://api.navitia.io/v1/coverage/fr-idf/lines?filter=line.code=4>
-   <https://api.navitia.io/v1/coverage/fr-idf/routes?filter=line.code=\"métro\ 347\">

### Few exploration examples

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/physical_modes' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 200 OK

{
    "links": [
        "..."
    ],
    "pagination": {
        "..."
    },
    "physical_modes": [
        {
            "id": "physical_mode:Bus",
            "name": "Bus"
        },
        {
            "id": "physical_mode:Metro",
            "name": "Métro"
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

But you will find lots of more advanced example in [a quick exploration](#a-quick-exploration)
chapter

<a name="pt-objects"></a>Autocomplete on Public Transport objects 
-----------------------------------------------------------------

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/pt_objects?q=metro%204&type\[\]=line&type\[\]=route' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 200 OK

{
"pt_objects": [
        {
            "embedded_type": "line",
            "line": {...},
            "id": "line:RAT:M4",
            "name": "RATP Metro 4 (Porte de Clignancourt - Mairie de Montrouge)"
        },
     ],
"links" : [...],
}
```


Also known as `/pt_objects` service.

This endpoint allows you to search in public transport objects using their names. It's a kind
of magical [autocomplete](https://en.wikipedia.org/wiki/Autocomplete) on public transport data. 
It returns a collection of [pt_object](#pt-object).

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

### Access

| url | Result |
|------------------------------------------------|-------------------------------------|
| `/coverage/{resource_path}/pt_objects`         | List of public transport objects    |


### Parameters


  Required | Name     | Type             | Description          | Default value
-----------|----------|------------------|----------------------|-----------------
  yep      | q        | string           | The search term      |
  nop      | type[]   | array of string  | Type of objects you want to query It takes one the following values: [`network`, `commercial_mode`, `line`, `route`, `stop_area`] | [`network`, `commercial_mode`, `line`, `route`, `stop_area`]

<aside class="warning">
There is no pagination for this api
</aside>


<a name="places"></a>Autocomplete on geographical objects
---------------------------------------------------------
``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/places?q=rue' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 200 OK

{
    "places": [
        {
        "embedded_type": "stop_area",
        "stop_area": {...},
        "id": "stop_area:RAT:SA:RDBAC",
        "name": "Rue du Bac (Paris)"
        },
        ...
    ],
    "links" : [...],
}
```

Also known as `/places` service.

This endpoint allows you to search in all geographical objects using their names, returning
a [place](#place) collection.

It is very useful to make some [autocomplete](https://en.wikipedia.org/wiki/Autocomplete) stuff ie 
to understand the user input even if he has mittens.

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

### Access

| url | Result |
|------------------------------------------------|-------------------------------------------------|
| `/coverage/{region_id}/places`                 | List of geographical objects within a coverage  |
| `/places`                                      | *Beta*: List of geographical objects within Earth |

<aside class="warning">
    Until now, you have to specify the right coverage to get `/places`.<br>
    If you like to play, you can test the "beta" `/places`, without any coverage: 
    it will soon be able to request entire Earth on adresses, POIs, stop areas... with geographical sort.
</aside>


### Parameters

  Required | Name      | Type        | Description            | Default value
  ---------|-----------|-------------|------------------------|-------------------
  yep      | q           | string    | The search term        |
  nop      | type[]      | array of string | Type of objects you want to query It takes one the following values: [`stop_area`, `address`, `administrative_region`, `poi`, `stop_point`] | [`stop_area`, `address`, `poi`, `administrative_region`]
  nop      | admin_uri[] | array of string | If filled, it will filter the search within the given admin uris



<a name="places-nearby-api"></a>Places Nearby
-----------------------------------------

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/stop_areas/stop_area:RAT:SA:CAMPO/places_nearby' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 200 OK

{
"places_nearby": [
    {
        "embedded_type": "stop_point",
        "stop_point": {...},
        "distance": "0",
        "quality": 0,
        "id": "stop_point:RAT:SP:CAMPO2",
        "name": "Campo-Formio (Paris)"
    },
    ....
}
```


Also known as `/places_nearby` service.

This endpoint allowe you to search for public transport objects that are near another object, or nearby
coordinates, returning a [places](#place) collection.

<aside class="warning">
    There is no pagination for this api.
</aside>

### Accesses

| url | Result |
|------------------------------------------------|-----------------------------------------------------------|
| `/coord/{lon;lat}/places_nearby`           | List of objects near the resource without any region id   |
| `/coverage/{lon;lat}/places_nearby`        | List of objects near the resource without any region id   |
| `/coverage/{resource_path}/places_nearby`  | List of objects near the resource                         |


### Parameters

  Required | name        | Type            | Description                       | Default value
  ---------|-------------|-----------------|-----------------------------------|---------------------------------------------------------------
  nop      | distance    | int             | Distance range in meters          | 500
  nop      | type[]      | array of string | Type of objects you want to query | [`stop_area`, `stop_point`, `poi`, `administrative_region`]
  nop      | admin_uri[] | array of string | If filled, will filter the search within the given admin uris       |
  nop      | filter      | string          | Use to filter returned objects. for example: places_type.id=theater |

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

<a name="journeys"></a>Journeys
-------------------------------

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/journeys?from=2.3749036;48.8467927&to=2.2922926;48.8583736' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'
```

``` shell
#response
HTTP/1.1 200 OK

{
    "tickets": [],
    "links": [...],
    "journeys": [
    {
        "fare": {...},
        "status": "",
        "tags": [],
        "type": "comfort",
        "nb_transfers": 0,
        "duration": 2671,
        "requested_date_time": "20160613T133748",
        "departure_date_time": "20160613T133830",
        "arrival_date_time": "20160613T142301",
        "calendars": [...],
        "co2_emission": {"unit": "gEC", "value": 24.642},
        "sections": [
        {
            "from": {... , "name": "Rue Abel"},
            "to": {... , "name": "Bercy (Paris)"},
            "arrival_date_time": "20160613T135400",
            "departure_date_time": "20160613T133830",
            "duration": 930,
            "type": "street_network",
            "mode": "walking",
            "geojson": {...},
            "path": [...],
            "links": []
        },{
            "from": {... , "name": "Bercy (Paris)"},
            "to": {... , "name": "Bir-Hakeim Tour Eiffel (Paris)"},
            "type": "public_transport",
            "display_informations": {
                "direction": "Charles de Gaulle — Étoile (Paris)",
                "code": "6",
                "color": "79BB92",
                "physical_mode": "M?tro",
                "headsign": "Charles de Gaulle Etoile",
                "commercial_mode": "Metro",
                "label": "6",
                "text_color": "000000",
                "network": "RATP"},
            "departure_date_time": "20160613T135400",
            "arrival_date_time": "20160613T141500",
            "base_arrival_date_time": "20160613T141500",
            "base_departure_date_time": "20160613T135400",
            "duration": 1260,
            "additional_informations": ["regular"],
            "co2_emission": {"unit": "gEC", "value": 24.642},
            "geojson": {...},
            "stop_date_times": [
            {
                "stop_point": {... , "label": "Bercy (Paris)"},
                "arrival_date_time": "20160613T135400",
                "departure_date_time": "20160613T135400",
                "base_arrival_date_time": "20160613T135400",
                "base_departure_date_time": "20160613T135400"
            },
            {...}
            ]
        },
        {
            "from": {... , "name": "Bir-Hakeim Tour Eiffel (Paris)" },
            "to": {... , "name": "Allée des Refuzniks"},
            "arrival_date_time": "20160613T142301",
            "departure_date_time": "20160613T141500",
            "duration": 481,
            "type": "street_network",
            "mode": "walking",
            "geojson": {...},
            "path": [...],
        }]
    },
    {...},
    {...}],
    "disruptions": [],
    "notes": [],
    "feed_publishers": [
    {
        "url": "",
        "id": "sandbox",
        "license": "",
        "name": ""
    }],
    "exceptions": []
}
```


Also known as `/journeys` service. This api computes journeys or isochrone tables.

There are two ways to access to this service : journeys from point to point, or isochrones from a single point to every point.

<aside class="success">
    Neither the 'from' nor the 'to' parameter of the journey are required,
    but obviously one of them has to be provided.
    <br>
    If only one is defined an isochrone is computed with every possible
    journeys from or to the point.
</aside>

### Accesses

| url | Result |
|--------------------------------------|-----------------------------------------|
| `/journeys`                          | List of journeys from wherever land     |
| `/coverage/{region_id}/journeys`     | List of journeys on a specific coverage |
| `/coverage/{a_path_to_resource}/journeys`     | Isochrone from a specific coverage |

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

####Requesting a single journey

The most used way to access to this service is to get the `/journeys` api endpoint.
Here is the structure of a standard journey request:

<https://api.navitia.io/v1/journeys?from={resource_id_1}&to={resource_id_2}&datetime={date_time_to_leave}> .

<a
    href="http://jsfiddle.net/gh/get/jquery/2.2.2/CanalTP/navitia/tree/documentation/slate/source/examples/jsFiddle/journeys/"
    target="_blank"
    class="button button-blue">
    Code it yourself on JSFiddle
</a>

In the [examples](#examples), positions are given by coordinates and no network is specified.
However when no coordinates are provided, you need to provide on what region you want to request as
<https://api.navitia.io/v1/coverage/us-ca/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682>

The list of regions covered by navitia is available through [coverage](#coverage).

<aside class="notice">
    If you want to use a specific data set, use the journey api within the
    data set: https://api.navitia.io/v1/coverage/{your_dataset}/journeys
</aside>

####Requesting an isochrone

If you want to retreive every possible journey from a single point at a time, you can request as follow:

<https://api.navitia.io/v1/{a_path_to_resource}/journeys> 

It will retrieve all the journeys from the resource (in order to make *[isochrone tables](https://en.wikipedia.org/wiki/Isochrone_map)*).

<a
    href="http://jsfiddle.net/gh/get/jquery/2.2.2/CanalTP/navitia/tree/documentation/slate/source/examples/jsFiddle/isochron/"
    target="_blank"
    class="button button-blue">
    Code it yourself on JSFiddle
</a>

The [isochrones](#isochrones) service exposes another response structure, which is simpler, for the same data.

### <a name="journeys-parameters"></a>Main parameters

| Required  | Name                    | Type          | Description                                                                            | Default value |
|-----------|-------------------------|---------------|----------------------------------------------------------------------------------------|---------------|
| nop       | from                    | id            | The id of the departure of your journey. If none are provided an isochrone is computed |               |
| nop       | to                      | id            | The id of the arrival of your journey. If none are provided an isochrone is computed   |               |
| yep       | datetime                | [iso-date-time](#iso-date-time) | Date and time to go                                                  |               |
| nop       | datetime_represents     | string        | Can be `departure` or `arrival`.<br>If `departure`, the request will retrieve journeys starting after datetime.<br>If `arrival` it will retrieve journeys arriving before datetime.                      | departure     |
| nop       | <a name="traveler-type"></a>traveler_type | enum | Define speeds and accessibility values for different kind of people.<br>Each profile also automatically determines appropriate first and last section modes to the covered area. Note: this means that you might get car, bike, etc fallback routes even if you set `forbidden_uris[]`! You can overload all parameters (especially speeds, distances, first and last modes) by setting all of them specifically. We advise that you don't rely on the traveler_type's fallback modes (`first_section_mode[]` and `last_section_mode[]`) and set them yourself.<br><div data-collapse><p>enum values:</p><ul><li>standard</li><li>slow_walker</li><li>fast_walker</li><li>luggage</li></ul></div>| standard      |
| nop       | data_freshness          | enum          | Define the freshness of data to use to compute journeys <ul><li>realtime</li><li>base_schedule</li></ul> _**when using the following parameter**_ "&data_freshness=base_schedule" <br> you can get disrupted journeys in the response. You can then display the disruption message to the traveler and make a realtime request to get a new undisrupted solution.   | base_schedule |
| nop       | forbidden_uris[]        | id            | If you want to avoid lines, modes, networks, etc.</br> Note: the forbidden_uris[] concern only the public transport objects. You can't for example forbid the use of the bike with them, you have to set the fallback modes for this (`first_section_mode[]` and `last_section_mode[]`) |               |
| nop       | first_section_mode[]    | array of string   | Force the first section mode if the first section is not a public transport one. It takes one the following values: `walking`, `car`, `bike`, `bss`.<br>`bss` stands for bike sharing system.<br>It's an array, you can give multiple modes.<br><br>Note: choosing `bss` implicitly allows the `walking` mode since you might have to walk to the bss station.<br> Note 2: The parameter is inclusive, not exclusive, so if you want to forbid a mode, you need to add all the other modes.<br> Eg: If you never want to use a `car`, you need: `first_section_mode[]=walking&first_section_mode[]=bss&first_section_mode[]=bike&last_section_mode[]=walking&last_section_mode[]=bss&last_section_mode[]=bike` | walking |
| nop       | last_section_mode[]     | array of string   | Same as first_section_mode but for the last section  | walking     |

### Other parameters

| Required | Name            | Type    | Description                   | Default value |
|----------|-----------------|---------|-------------------------------|---------------|
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

<a name="isochrones_api"></a>Isochrones (currently in Beta)
---------------------------------------

Also known as `/isochrones` service.

<aside class="warning">
    This service is under development. So it is accessible as a <b>"Beta" service</b>. 
    <br>
    Every feed back is welcome on <a href="https://groups.google.com/forum/#!forum/navitia">https://groups.google.com/forum/#!forum/navitia</a> !
</aside>


This service gives you a multi-polygon response which 
represent a same time travel zone: https://en.wikipedia.org/wiki/Isochrone_map

As you can find isochrone tables using `/journeys`, this service is only another representation 
of the same data, map oriented.

It is also really usefull to make filters on geocoded objects in order to find which ones are reachable within a specific time.
You just have to verify that the coordinates of the geocoded object are inside the multi-polygon.

### Accesses

| url | Result |
|------------------------------------------|-------------------------------------|
| `/isochrones`                          | List of multi-polygons representing one isochrone step. Access from wherever land |
| `/coverage/{region_id}/isochrones` | List of multi-polygons representing one isochrone step. Access from on a specific coverage |

### <a name="isochrones-parameters"></a>Main parameters

<aside class="success">
    'from' and 'to' parameters works as exclusive parameters:
    <li>When 'from' is provided, 'to' is ignore and Navitia computes a "departure after" isochrone.
    <li>When 'from' is not provided, 'to' is required and Navitia computes a "arrival after" isochrone.
</aside>



| Required  | Name                    | Type          | Description                                                                               | Default value |
|-----------|-------------------------|---------------|-------------------------------------------------------------------------------------------|---------------|
| nop       | from                    | id            | The id of the departure of your journey. Required to compute isochrones "departure after" |               |
| nop       | to                      | id            | The id of the arrival of your journey. Required to compute isochrones "arrival before"    |               |
| yep       | datetime                | [iso-date-time](#iso-date-time) | Date and time to go                                                                          |               |
| nop       | forbidden_uris[]        | id            | If you want to avoid lines, modes, networks, etc.</br> Note: the forbidden_uris[] concern only the public transport objects. You can't for example forbid the use of the bike with them, you have to set the fallback modes for this (`first_section_mode[]` and `last_section_mode[]`)                                                 |               |
| nop       | first_section_mode[]    | array of string   | Force the first section mode if the first section is not a public transport one. It takes one the following values: `walking`, `car`, `bike`, `bss`.<br>`bss` stands for bike sharing system.<br>It's an array, you can give multiple modes.<br><br>Note: choosing `bss` implicitly allows the `walking` mode since you might have to walk to the bss station.<br> Note 2: The parameter is inclusive, not exclusive, so if you want to forbid a mode, you need to add all the other modes.<br> Eg: If you never want to use a `car`, you need: `first_section_mode[]=walking&first_section_mode[]=bss&first_section_mode[]=bike&last_section_mode[]=walking&last_section_mode[]=bss&last_section_mode[]=bike` | walking |
| nop       | last_section_mode[]     | array of string   | Same as first_section_mode but for the last section  | walking     |

Other parameters to come...

<a name="route-schedules"></a>Route Schedules and time tables
-------------------------------------------------------------

Also known as `/route_schedules` service.

This endpoint gives you access to schedules of routes, with a response made
of an array of [route_schedule](#route-schedule), and another one of [note](#note). You can
access it via that kind of url: <https://api.navitia.io/v1/{a_path_to_a_resource}/route_schedules>

### Accesses

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `/coverage/{resource_path}/route_schedules`  | List of the entire route schedules for a given resource                                                       |
| `/coverage/{lon;lat}/route_schedules`        | List of the entire route schedules for coordinates                                                       |

### Parameters

Required | Name               | Type      | Description                                                                              | Default Value
---------|--------------------|-----------|------------------------------------------------------------------------------------------|--------------
yep      | from_datetime      | [iso-date-time](#iso-date-time) | The date_time from which you want the schedules                    |
nop      | duration           | int       | Maximum duration in seconds between from_datetime and the retrieved datetimes.           | 86400
nop      | items_per_schedule | int       | Maximum number of columns per schedule.                                                  |
nop      | forbidden_uris[]   | id        | If you want to avoid lines, modes, networks, etc.                                        | 
nop      | data_freshness     | enum      | Define the freshness of data to use<br><ul><li>realtime</li><li>base_schedule</li></ul>  | base_schedule


### Objects

#### <a name="route-schedule">route_schedule object

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

This endpoint gives you access to time tables going through a stop
point as:
![stop_schedules](https://upload.wikimedia.org/wikipedia/commons/thumb/7/7d/Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg/640px-Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg)

The response is made of an array of [stop_schedule](#stop-schedule), and another
one of [note](#note). 
You can access it via that kind of url: <https://api.navitia.io/v1/{a_path_to_a_resource}/stop_schedules>

### Accesses

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `/coverage/{lon;lat}/stop_schedules`        | List of the stop schedules grouped by ``stop_point/route`` for coordinates                                    |
| `/coverage/{resource_path}/stop_schedules`  | List of the stop schedules grouped by ``stop_point/route`` for a given resource                               |


### Parameters

Required | Name           | Type                    | Description        | Default Value
---------|----------------|-------------------------|--------------------|--------------
yep      | from_datetime  | [iso-date-time](#iso-date-time) | The date_time from which you want the schedules |
nop      | duration         | int                            | Maximum duration in seconds between from_datetime and the retrieved datetimes.                            | 86400
nop      | forbidden_uris[] | id                             | If you want to avoid lines, modes, networks, etc.    | 
nop      | items_per_schedule | int       | Maximum number of datetimes per schedule.                                                  |
nop      | data_freshness   | enum                           | Define the freshness of data to use to compute journeys <ul><li>realtime</li><li>base_schedule</li></ul> | base_schedule


### <a name="stop-schedule"></a>Stop_schedule object

|Field|Type|Description|
|-----|----|-----------|
|display_informations|[display_informations](#display-informations)|Usefull information about the route to display|
|route|[route](#route)|The route of the schedule|
|date_times|Array of [pt-date-time](#pt-date-time)|When does a bus stops at the stop point|
|stop_point|[stop_point](#stop-point)|The stop point of the schedule|
|additional_informations|[additional_informations](#additional-informations)|Other informations, when no departures <div data-collapse><p>enum values:</p><ul><li>date_out_of_bounds</li><li>terminus</li><li>partial_terminus</li><li>active_disruption</li><li>no_departures_known</li></ul></div>|


<a name="departures"></a>Departures
-----------------------------------

Also known as `/departures` service.

This endpoint retrieves a list of departures from a specific datetime of a selected
object. 
Departures are ordered chronologically in ascending order as:
![departures](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/Display_at_bus_stop_sign_in_Karlovo_n%C3%A1m%C4%9Bst%C3%AD%2C_T%C5%99eb%C3%AD%C4%8D%2C_T%C5%99eb%C3%AD%C4%8D_District.JPG/640px-Display_at_bus_stop_sign_in_Karlovo_n%C3%A1m%C4%9Bst%C3%AD%2C_T%C5%99eb%C3%AD%C4%8D%2C_T%C5%99eb%C3%AD%C4%8D_District.JPG)


### Accesses

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `/coverage/{resource_path}/departures`      | List of the next departures, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here) |
| `/coverage/{lon;lat}/departures`            | List of the next departures, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here) |

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
|stop_date_time|Array of [stop_date_time](#stop_date_time)|Occurs when a bus does a stopover at the stop point|
|stop_point|[stop_point](#stop-point)|The stop point of the schedule|

<a name="arrivals"></a>Arrivals
-------------------------------

``` shell
curl 'https://api.navitia.io/v1/coverage/sandbox/stop_areas/stop_area:RAT:SA:GDLYO/arrivals' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

HTTP/1.1 200 OK

{
    "arrivals":[
        {
            "display_informations":{
                "direction":"Saint-Lazare (Paris)",
                "code":"14",
                "color":"67328E",
                "physical_mode":"Métro",
                "headsign":"Olympiades",
                "commercial_mode":"Metro",
                "network":"RATP"
            },
            "stop_date_time":{
                "arrival_date_time":"20160615T115400",
                "departure_date_time":"20160615T115400",
                "base_arrival_date_time":"20160615T115400",
                "base_departure_date_time":"20160615T115400",
                "data_freshness":"base_schedule"
            },
            "stop_point":{
                "id":"stop_point:RAT:SP:GDLYO4",
                "name":"Gare de Lyon",
                "label":"Gare de Lyon (Paris)"
            },
            "route":{
                "id":"route:RAT:M14_R",
                "name":"Olympiades - Gare Saint-Lazare"
            }
        },
        {"...": "..."},
        {"...": "..."}
    ]
}
```

Also known as `/arrivals` service.

This endpoint retrieves a list of arrivals from a specific datetime of a selected
object. Arrivals are ordered chronologically in ascending order.

### Accesses

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `/coverage/{resource_path}/arrivals`        | List of the arrivals, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here)        |
| `/coverage/{lon;lat}/arrivals`              | List of the arrivals, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here)        |

### Parameters

they are exactly the same as [departures](#departures)

<a name="traffic-reports"></a>Traffic reports
---------------------------------------------
``` shell
#request
$ curl 'http://api.navitia.io/v1/coverage/sandbox/traffic_reports' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response, composed by 2 main lists: "traffic_reports" and "disruptions"
HTTP/1.1 200 OK

{
"traffic_reports": [
        "network": {
        #main object (network) and links within its own disruptions
        },
        "lines": [
        #list of all disrupted lines from the network and disruptions links
        ],
        "stop_areas": [
        #list of all disrupted stop_areas from the network and disruptions links
        ],
    ],[
        #another network with its lines and stop areas
    ],
"disruptions": [...]
}
```

Also known as `/traffic_reports` service.

This service provides the state of public transport traffic. It can be
called for an overall coverage or for a specific object.

![Traffic reports](/images/traffic_reports.png)

### Parameters

You can access it via that kind of url: <https://api.navitia.io/v1/{a_path_to_a_resource}/traffic_reports>

For example:

-   overall public transport traffic report on Ile de France coverage
    -   <https://api.navitia.io/v1/coverage/fr-idf/traffic_reports>
-   Is there any perturbations on the RER network ?
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/traffic_reports>
-   Is there any perturbations on the "RER A" line ?
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/lines/line:TRN:DUA810801043/traffic_reports>


The response is made of an array of [traffic_reports](#traffic-reports), 
and another one of [disruptions](#disruption). 

There are inner links between this 2 arrays: 
see the [inner-reference](#inner-references) section to use them.


### Traffic report object
``` shell
#links between objects in a traffic_reports response
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

Traffic_reports is an array of some traffic_report object.

One traffic_report object is a complex object, made of a network, an array
of lines and an array of stop_areas. 

A typical traffic_report object will contain:

-   1 network which is the grouping object
    -   it can contain links to its disruptions. These disruptions are globals and might not be applied on lines or stop_areas.
-   0..n lines
    -   each line contains at least a link to its disruptions
-   0..n stop_areas
    -   each stop_area contains at least a link to its disruptions

It means that if a stop_area is used by many networks, it will appear
many times.


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


