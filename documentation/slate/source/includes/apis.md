API catalog
===========

<h2 id="coverage">Coverage</h2>

Also known as `/coverage` service.

You can easily navigate through regions covered by navitia.io, with the
coverage api. The shape of the region is provided in GeoJSON.

In Navitia, a coverage is a combination of multiple [datasets](#datasets)
provided by different [contributors](#contributors)
(typically data provided by a transport authority in GTFS format).
The combination of datasets used by a coverage is arbitrarily determined
but we try to use something that makes sense and has a reasonnable amount of data (country).

The only arguments are the ones of [paging](#paging).

<aside class="notice">
    Most of the time you can avoid providing a region and let Navitia guess the right one for you.
    This can be done by providing coordinates in place of the region name:
    `/coverage/{lon;lat}/endpoint?parameter=value`
</aside>

### Accesses

| url                                      | Result                                                                           |
|------------------------------------------|----------------------------------------------------------------------------------|
| `/coverage`                              | List of the areas covered by navitia                                             |
| `/coverage/{region_id}`                  | Information about a specific region                                              |
| `/coverage/{lon;lat}`                    | Information about a specific region, navitia guesses the region from coordinates |

### Fields

| Field                | Type                       | Description                                |
|----------------------|----------------------------|--------------------------------------------|
|id                    |string                      | Identifier of the coverage                 |
|name                  |string                      | Name of the coverage                       |
|shape                 |string                      | GeoJSON of the shape of the coverage       |
|dataset_created_at    |[iso-date-time](#iso-date-time) | Creation date of the dataset           |
|start_production_date |[iso-date](#iso-date)       | Beginning of the production period. We only have data on this production period |
|end_production_date   |[iso-date](#iso-date)       | End of the production period. We only have data on this production period |

### Production period

The production period is the validity period of the coverage's data.

There is no data outside this production period.

This production period cannot exceed one year.

<aside class="notice">
Navitia need transportation data to work and those are very date dependant. To check that a coverage has some data for a given date, you need to check the production period of the coverage.
</aside>

<h2 id="datasets">Datasets</h2>

Very simple endpoint providing the sets of data that are used in the given coverage.

Those datasets (typically from transport authority in GTFS format), each provided by a
unique [contributor](#contributors) are forming a [coverage](#coverage).

Contributor providing the dataset is also provided in the response.
Very useful to know all the data that form a coverage.

The only arguments are the ones of [paging](#paging).

### Accesses

| url | Result |
|----------------------------------------------|-------------------------------------------|
| `coverage/{region_id}/datasets`              | List of the datasets of a specific region |
| `coverage/{region_id}/datasets/{dataset_id}` | Information about a specific dataset      |


<h2 id="contributors">Contributors</h2>

Very simple endpoint providing the contributors of data for the given coverage.

A contributor is a data provider (typically a transport authority), and can provide multiple [datasets](#datasets).
For example, the contributor Italian Railways will provide a dataset for the national train and some others for the regional trains.
We will try to put them in the same [coverage](#coverage) so that we assemble them in the same journey search, using both.

Very usefull to know which contributors are used in the datasets forming a coverage.

The only arguments are the ones of [paging](#paging).

### Accesses

| url | Result |
|--------------------------------------------------|-----------------------------------------------|
| `coverage/{region_id}/contributors`              | List of the contributors of a specific region |
| `coverage/{region_id}/contributors/{dataset_id}` | Information about a specific contributor      |


<h2 id="coord">Inverted geocoding</h2>

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

### Accesses

| url                                          | Result                                                               |
|----------------------------------------------|----------------------------------------------------------------------|
| `places/{lon;lat}`                           | Detailed address point                                               |
| `/places/{id}`                               | Information about places                                             |
| `coverage/{lon;lat}/places/{lon;lat}`        | Detailed address point, navitia guesses the region from coordinates  |
| `coverage/{lon;lat}/places/{id}`             | Information about places, navitia guesses the region from coordinates|
| `coverage/{region_id}/places/{lon;lat}`      | Detailed address point                                               |
| `coverage/{region_id}/places/{id}`           | Information about places                                             |


You can also combine `/coords` with other filter as:

-   get [POIs](#poi) near a coordinate
	- <https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/pois?distance=1000>
-   get specific POIs near a coordinate
	- <https://api.navitia.io/v1/coverage/fr-idf/poi_types/poi_type:amenity:bicycle_rental/coords/2.377310;48.847002/pois?distance=1000>

<h2 id="pt-ref">Public Transportation Objects exploration</h2>

>[Try it on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fpt_objects%3Fq%3Dmetro%25201)

``` shell
curl 'https://api.navitia.io/v1/coverage/sandbox/pt_objects?q=metro%201' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

HTTP/1.1 200 OK

{
    "pt_objects":[
        {
            "id":"line:RAT:M1",
            "name":"RATP Metro 1 (Château de Vincennes - La Défense)",
            "embedded_type":"line",
            "line":{
                "id":"line:RAT:M1",
                "name":"Château de Vincennes - La Défense",
                "code":"1",
                "...": "..."
            }
        },
        {
            "id":"line:RAT:M11",
            "name":"RATP Metro 11 (Mairie des Lilas - Châtelet)"
            "embedded_type":"line",
            "line":{
                "...": "..."
            },
        },
        {
            "id":"line:RAT:M12",
            "name":"RATP Metro 12 (Mairie d'Issy - Front Populaire)",
            "embedded_type":"line",
            "line":{
                "...": "..."
            }
        },
        {"...": "..."},
        {"...": "..."}
    ]
}
```

Also known as `/networks`, `/lines`, `/stop_areas`... services.

Once you have selected a region, you can explore the public
transportation objects easily with these APIs. You just need to add at
the end of your URL a collection name to see every objects within a
particular collection. To see an object detail, add the id of this object at the
end of the collection's URL. The [paging](#paging) arguments may be used to
paginate results.

### Accesses

| url                                                     | Result                                                                           |
|---------------------------------------------------------|----------------------------------------------------------------------------------|
| `/coverage/{region_id}/{collection_name}`               | Collection of objects of a region                                                |
| `/coverage/{region_id}/{collection_name}/{object_id}`   | Information about a specific object                                              |
| `/coverage/{lon;lat}/{collection_name}`                 | Collection of objects of a region, navitia guesses the region from coordinates |
| `/coverage/{lon;lat}/{collection_name}/{object_id}`     | Information about a specific object, navitia guesses the region from coordinates |

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

### Shared parameters

#### <a name="depth"></a>depth

You are looking for something, but Navitia doesn't output it in your favorite endpoint?<br>You want to request more from navitia feed?<br>You are receiving feeds that are too important and too slow with low bandwidth?<br>You would like Navitia to serve GraphQL but it is still not planned?

Feeds from endpoint might miss informations, but this tiny `depth=` parameter can
expand Navitia power by making it more wordy. Or lighter if you want it.

Here is some examples around "metro line 1" from the Parisian network:

- Get "line 1" id
	- <https://api.navitia.io/v1/coverage/sandbox/pt_objects?q=metro%201>
	The id is "line:RAT:M1"
- Get routes for this line
	- <https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/routes>
	Default depth is `depth=1`
- Want to get a tiny response? Just add `depth=0`
	- <https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/routes?depth=0>
	The response is lighter (parent lines disappear for example)
- Want more informations, just add `depth=2`
	- <https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/routes?depth=2>
	The response is a little more verbose (some geojson can appear in response when using your open data token)
- Wanna fat more informations, let's try `depth=3`
	- <https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/routes?depth=3>
	Big response: all stop_points are shown
- Wanna spam the internet bandwidth? Try `depth=42`
	- No. There is a technical limit with `depth=3`

#### odt level

-   Type: String
-   Default value: all
-   Warning: works ONLY with */[lines](#line)* collection...

It allows you to request navitia for specific pickup lines. It refers to
the [odt](#odt) section. "odt_level" can take one of these values:

-   all (default value): no filter, provide all public transport lines,
    whatever its type
-   scheduled: provide only regular lines (see the [odt](#odt) section)
-   with_stops: to get regular, "odt_with_stop_time" and "odt_with_stop_point" lines.
    -   You can easily request route_schedule and stop_schedule with these kind of lines.
    -   Be aware of "estimated" stop times
-   zonal: to get "odt_with_zone" lines with non-detailed journeys

For example

<https://api.navitia.io/v1/coverage/fr-nw/networks/network:lila/lines>

<https://api.navitia.io/v1/coverage/fr-nw/networks/network:irigo/lines?odt_level=scheduled>

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

-   <https://api.navitia.io/v1/coverage/fr-idf/vehicle_journeys?headsign=PADO>
-   <https://api.navitia.io/v1/coverage/fr-idf/stop_areas?headsign=PADO>


<aside class="warning">
    This last request gives the stop areas used by the vehicle
    journeys containing the headsign PADO, <b>not</b> the stop areas where it
    exists a stop time with the headsign PADO.
</aside>

#### since / until

-   Type: [iso-date-time](#iso-date-time)

To be used only on "vehicle_journeys" and "disruptions" collection, to filter on a
period. Both parameters "until" and "since" are optional.

For vehicle_journeys, "since" and "until" are associated with the data_freshness parameter (defaults to base_schedule): see the [realtime](#realtime) section.

Example:

-   Getting every active (only base_schedule) New Jersey vehicles between 12h00 and 12h01, on a specific date <https://api.navitia.io/v1/coverage/us-ny/networks/network:newjersey/vehicle_journeys?since=20170407T120000&until=20170407T120100>
-   Getting every active (according to realtime information) New Jersey vehicles between 12h00 and 12h01, on a specific date <https://api.navitia.io/v1/coverage/us-ny/networks/network:newjersey/vehicle_journeys?since=20170407T120000&until=20170407T120100&data_freshness=realtime>
-   Getting every active disruption on "Bretagne" for a specific date <https://api.navitia.io/v1/coverage/fr-bre/disruptions?since=20170206000000&until=20170206235959>

<aside class="warning">
    On vehicle_journey this filter is applied using only the first stop time.
    On disruption this filter must intersect with one application period.
    "since" and "until" are included.
</aside>

#### disable_geojson

By default geojson part of an object are returned in navitia's responses, this parameter allows you to
remove them, it's useful when searching lines that you don't want to display on a map.

<aside class="notice">
    Geojson objects can be very large, 1MB is not unheard of, and they don't compress very well.
    So this parameter is mostly here for reducing your downloading times and helping your json parser.
    It's almost mandatory on mobile devices since most cellular networks are still relatively slow.
</aside>

Examples:

-   <https://api.navitia.io/v1/coverage/fr-idf/lines?disable_geojson=true>

#### disable_disruption

By default disruptions are also present in navitia's responses on apis "PtRef", "pt_objects" and "places_nearby".
This parameter allows you to remove them, reducing the response size.

<aside class="notice">
    Disruptions can be large and not necessary while searching objects.
    This parameter is mostly here to be able to search objects without disruptions in the response.
</aside>

Examples:

-   <https://api.navitia.io/v1/coverage/fr-idf/lines?disable_disruption=true>

### <a name="filter"></a>Filter

It is possible to apply a filter to the returned collection, using
"filter" parameter. If no object matches the filter, a "bad_filter"
error is sent. If filter can not be parsed, an "unable_to_parse" error
is sent.

#### {collection}.has_code

>[Try it on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%3Ffilter%3Dstop_area.has_code(source%252CSA%253ACAMPO)%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

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

Examples:

-   <https://api.navitia.io/v1/coverage/fr-sw/stop_points?filter=stop_point.has_code(source,5852)>
-   <https://api.navitia.io/v1/coverage/fr-sw/stop_areas?filter=stop_area.has_code(gtfs_stop_code,1303)>
-   <https://api.navitia.io/v1/coverage/fr-sw/lines?filter=line.has_code(source,11821949021891619)>

<aside class="warning">
    these ids (which are not Navitia ids) may not be unique. you will have to manage a tuple in response.
</aside>

#### line.code

It allows you to request navitia objects referencing a line whose code
is the one provided, especially lines themselves and routes.

Examples:

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

You will find lots of more advanced example in [a quick exploration](#a-quick-exploration)
chapter

<h2 id="pt-objects">Autocomplete on Public Transport objects</h2>

>[Try it on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fpt_objects%3Fq%3Dmetro%25204%26type%5B%5D%3Dline%26type%5B%5D%3Droute)


``` shell
# Search objects of type 'line' or 'route' containing 'metro 4'

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

Different kinds of objects can be returned (sorted as):

-   network
-   commercial_mode
-   line
-   route
-   stop_area
-   stop_point

Here is a typical use case. A traveler has to find a line between the
1500 lines around Paris.

#### Examples

User could type one of the following without any filters:

##### Traveler input "bob":

-  network : "bobby network"
-  line : "bobby bus 1"
-  line : "bobby bus 2"
-  route : "bobby bus 1 to green"
-  route : "bobby bus 1 to rose"
-  route : "bobby bus 2 to yellow"
-  stop_area : "...

##### Traveler input "bobby met":

-  line : "bobby metro 1"
-  line : "bobby metro 11"
-  line : "bobby metro 2"
-  line : "bobby metro 3"
-  route : "bobby metro 1 to Martin"
-  route : "bobby metro 1 to Mahatma"
-  route : "bobby metro 11 to Marcus"
-  route : "bobby metro 11 to Steven"
-  route : "...

##### Traveler input: "bobby met 11" or "bobby metro 11":

-  line : "bobby metro 11"
-  route : "bobby metro 11 to Marcus"
-  route : "bobby metro 11 to Steven"

### Access

``` shell
# Search objects of type 'network' containing 'RAT'
curl 'https://api.navitia.io/v1/coverage/sandbox/pt_objects?q=RAT&type\[\]=network' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

HTTP/1.1 200 OK

{
    "pt_objects":[
        {
            "id":"network:RAT:1",
            "name":"RATP",
            "embedded_type":"network",
            "network":{
                "id":"network:RAT:1",
                "name":"RATP"
            }
        }
    ]
}
```

| url | Result |
|-------------------------------------------------------|-------------------------------------|
| `/coverage/{region_id}/{resource_path}/pt_objects`    | List of public transport objects    |

### Parameters


  Required | Name     | Type             | Description          | Default value
-----------|----------|------------------|----------------------|-----------------
  yep      | q        | string           | The search term      |
  nop      | type[]   | array of string  | Type of objects you want to query It takes one the following values: [`network`, `commercial_mode`, `line`, `route`, `stop_area`, `stop_point`] | [`network`, `commercial_mode`, `line`, `route`, `stop_area`]
  nop      | disable_disruption | boolean  | Remove disruptions from the response  | False
  nop      | depth    | int              | Json response [depth](#depth) | 1
  nop      | filter   | string  | Use to filter returned objects. for example: network.id=sncf |

<aside class="warning">
There is no pagination for this api
</aside>

<h2 id="places">Autocomplete on geographical objects</h2>

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
-   stop_point (appears only if specified, using `&type[]=stop_point` filter)

<aside class="warning">
    There is no pagination for this api.
</aside>

<aside class="notice">
    The returned `quality` field is deprecated and only maintained for backward compatibility.
    Please consider only navitia's output order.
</aside>

### Access

| url | Result |
|------------------------------------------------|-------------------------------------------------|
| `/coverage/{region_id}/places`                 | List of geographical objects within a coverage  |
| `/places`                                      | *Beta*: List of geographical objects within Earth |

<aside class="warning">
    Until now, you have to specify the right coverage to get `/places`.<br>
    If you like to play, you can test the "beta" `/places`, without any coverage:
    it will soon be able to request entire Earth on addresses, POIs, stop areas... with geographical sort.
</aside>

### Parameters

  Required | Name      | Type        | Description            | Default value
  ---------|-----------|-------------|------------------------|-------------------
  yep      | q           | string    | The search term        |
  nop      | type[]      | array of string | Type of objects you want to query It takes one the following values: [`stop_area`, `address`, `administrative_region`, `poi`, `stop_point`] | [`stop_area`, `address`, `poi`, `administrative_region`]
  nop      | ~~admin_uri[]~~ |   | Deprecated. Filters on shape are now possible straight in user account
  nop      | disable_geojson | boolean | remove geojson from the response | False
  nop      | depth       | int             | Json response [depth](#depth)     | 1
  nop      | from | string | Coordinates longitude;latitude used to prioritize the objects around this coordinate. Note this parameter will be taken into account only if the autocomplete's backend can handle it |


<h2 id="places-nearby-api">Places nearby</h2>

>[Try it on Navitia playground (click on "MAP" buttons to see places)](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%3ARAT%3ASA%3ACAMPO%2Fplaces_nearby)

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

This endpoint allows you to search for public transport objects that are near another object, or nearby
coordinates, returning a [places](#place) collection.

### Accesses

| url                                                    | Result                                                                         |
|--------------------------------------------------------|--------------------------------------------------------------------------------|
| `/coverage/{lon;lat}/coords/{lon;lat}/places_nearby`   | List of objects near the resource, navitia guesses the region from coordinates |
| `/coord/{lon;lat}/places_nearby`                       | List of objects near the resource without any region id (same result as above) |
| `/coverage/{region_id}/coords/{lon;lat}/places_nearby` | List of objects near a coordinate                                              |
| `/coverage/{region_id}/{resource_path}/places_nearby`  | List of objects near the resource                                              |

### Parameters

  Required | name        | Type            | Description                       | Default value
  ---------|-------------|-----------------|-----------------------------------|---------------------------------------------------------------
  nop      | distance    | int             | Distance range in meters          | 500
  nop      | type[]      | array of string | Type of objects you want to query | [`stop_area`, `stop_point`, `poi`]
  nop      | admin_uri[] | array of string | If filled, will filter the search within the given admin uris       |
  nop      | filter      | string          | Use to filter returned objects. for example: places_type.id=theater |
  nop      | disable_geojson | boolean     | Remove geojson from the response  | False
  nop      | disable_disruption | boolean  | Remove disruptions from the response  | False
  nop      | count       | int             | Elements per page                 | 10
  nop      | depth       | int             | Json response [depth](#depth)     | 1
  nop      | start_page  | int             | The page number (cf the [paging section](#paging)) | 0
  nop      | add_poi_infos[] | enum        | Activate the output of additional infomations about the poi. For example, parking availability (BSS, car parking etc.) in the pois of response. Pass `add_poi_infos[]=none&` or `add_poi_infos[]=&` (empty string) to deactivate all.   | [`bss_stands`, `car_park`]

Filters can be added:

-   request for the city of "Paris" on fr-idf
    -   <https://api.navitia.io/v1/coverage/fr-idf/places?q=paris>
-   then pois nearby this city
    -   <https://api.navitia.io/v1/coverage/fr-idf/places/admin:7444/places_nearby>
-   and then, let's catch every parking around
    -   "distance=10000" Paris is not so big
    -   "type[]=poi" to take pois only
    -   "filter=poi_type.id=poi_type:amenity:parking" to get parking
    -   "count=100" for classic pagination (to get the 100 nearest ones)
    -   <https://api.navitia.io/v1/coverage/fr-idf/places/admin:7444/places_nearby?distance=10000&count=100&type[]=poi&filter=poi_type.id=poi_type:amenity:parking>

The results are sorted by distance.

<h2 id="journeys">Journeys</h2>

>[Try it on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fjourneys%3Ffrom%3D2.3749036%3B48.8467927%26to%3D2.2922926%3B48.8583736)

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

There are two ways to access to this service: journeys from point to point, or isochrones from a single point to every point.

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

#### Requesting a single journey

The most used way to access to this service is to get the `/journeys` api endpoint.
Here is the structure of a standard journey request:

<https://api.navitia.io/v1/journeys?from={resource_id_1}&to={resource_id_2}&datetime={date_time_to_leave}> .

[Context](#context) object provides the `current_datetime`, useful to compute waiting time when requesting Navitia without a `datetime`.

<aside class="success">
    By default journeys are computed considering a traveler that walks at the beginning and the end.
    <br>
    This can be modified using parameters "first_section_mode" and "last_section_mode" that are arrays.
    <br>
    Example allowing bike or walk at the beginning: <a href="https://api.navitia.io/v1/journeys?from=2.3865494;48.8499182&to=2.3643739;48.854&first_section_mode[]=walking&first_section_mode[]=bike">https://api.navitia.io/v1/journeys?from=2.3865494;48.8499182&to=2.3643739;48.854&first_section_mode[]=walking&first_section_mode[]=bike</a>
</aside>

<a
    href="https://jsfiddle.net/kisiodigital/0oj74vnz/"
    target="_blank">
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

#### Requesting an isochrone

If you want to retreive every possible journey from a single point at a time, you can request as follow:

<https://api.navitia.io/v1/{a_path_to_resource}/journeys>

It will retrieve all the journeys from the resource (in order to make *[isochrone tables](https://en.wikipedia.org/wiki/Isochrone_map)*).

<a
    href="https://jsfiddle.net/kisiodigital/x6207t6f/"
    target="_blank">
    Code it yourself on JSFiddle
</a>

The [isochrones](#isochrones) service exposes another response structure, which is simpler, for the same data.

<h3 id="journeys-disruptions">Disruptions</h3>

By default, Navitia only computes journeys without their associated disruption(s), meaning that the journeys in the response will be based on the theoretical schedules. The disruption present in the response is for information only.
In order to get an "undisrupted" journey (consider all disruptions during journey planning), you just have to add a `&data_freshness=realtime` parameter (or use the `bypass_disruptions` link from response).

In a journey's response, different disruptions may have different meanings.
Each journey has a `status` attribute that indicates the actual effect affecting pick-up and drop-off used by
the journey (no matter the effects of the disruptions attached to the journey).
A journey using a stop-time pick-up (or drop-off) that is deleted in realtime will have a `NO_SERVICE` status.
A journey using a stop-time pick-up (or drop-off) that is added in realtime will have a `MODIFIED_SERVICE` status.
A journey using a stop-time pick-up (or drop-off) that is early or late in realtime will have a `SIGNIFICANT_DELAYS` status.
All other journeys will have an empty status.
Disruptions are on the sections, the ones that impact the journey are in the sections's display_informations links  (`sections[].display_informations.links[]`).

You might also have other disruptions in the response. They don't directly impact the journey, but might affect them.
For example, some intermediate stops of a section can be disrupted, it doesn't prevent the journey from being realised but modifies it.
These disruptions won't be on the `display_informations` of the sections or used in the journey's status.

See how disruptions affect a journey in the [real time](#realtime) section.

<h3 id="journeys-parameters">Main parameters</h3>

| Required  | Name                    | Type          | Description                                                                            | Default value |
|-----------|-------------------------|---------------|----------------------------------------------------------------------------------------|---------------|
| nop       | from                    | id            | The id of the departure of your journey. If none are provided an isochrone is computed. Should be different than `to` or no journey will be computed.  |               |
| nop       | to                      | id            | The id of the arrival of your journey. If none are provided an isochrone is computed. Should be different than `from` or no journey will be computed. |               |
| nop       | datetime                | [iso-date-time](#iso-date-time) | Date and time to go.<br>Note: the datetime must be in the [coverage's publication period](#coverage)                                                   | now           |
| nop       | datetime_represents     | string        | Can be `departure` or `arrival`.<br>If `departure`, the request will retrieve journeys starting after datetime.<br>If `arrival` it will retrieve journeys arriving before datetime.                      | departure     |
| nop       | <a name="traveler-type"></a>traveler_type | enum | Define speeds and accessibility values for different kind of people.<br>Each profile also automatically determines appropriate first and last section modes to the covered area. Note: this means that you might get car, bike, etc fallback routes even if you set `forbidden_uris[]`! You can overload all parameters (especially speeds, distances, first and last modes) by setting all of them specifically.<br> We advise that you don't rely on the traveler_type's fallback modes (`first_section_mode[]` and `last_section_mode[]`) and set them yourself.<br>Enum values:<ul><li>standard</li><li>slow_walker</li><li>fast_walker</li><li>luggage</li><li>wheelchair</li></ul>|               |
| nop       | data_freshness          | enum          | Define the freshness of data to use to compute journeys <ul><li>realtime</li><li>base_schedule</li></ul> _**when using the following parameter**_ "&data_freshness=base_schedule" <br> you can get disrupted journeys in the response. You can then display the disruption message to the traveler and make a realtime request to get a new "undisrupted" solution (considering all disruptions during journey planning).   | base_schedule |
| nop       | forbidden_uris[]        | id            | If you want to avoid lines, modes, networks, etc.</br> Note: the forbidden_uris[] concern only the public transport objects. You can't for example forbid the use of the bike with them, you have to set the fallback modes for this (`first_section_mode[]` and `last_section_mode[]`) |               |
|nop        | allowed_id[]            | id            | If you want to use only a small subset of the public transport objects in your solution. The constraint intersects with `forbidden_uris[]`. For example, if you ask for `allowed_id[]=line:A&forbidden_uris[]=physical_mode:Bus`, only vehicles of the line A that are not buses will be used. | everything |
| nop       | first_section_mode[]    | array of string   | Force the first section mode if the first section is not a public transport one. It takes the following values: `walking`, `car`, `bike`, `bss`, `ridesharing`, `taxi`.<br>It's an array, you can give multiple modes.<br><br>See [Ridesharing](#ridesharing-stuff) and [Taxi](#taxi-stuff) sections for more details on these modes.<br>`bss` stands for bike sharing system.<br>Note: choosing `bss` implicitly allows the `walking` mode since you might have to walk to the bss station.<br> Note 2: The parameter is inclusive, not exclusive, so if you want to forbid a mode, you need to add all the other modes.<br> Eg: If you never want to use a `car`, you need: `first_section_mode[]=walking&first_section_mode[]=bss&first_section_mode[]=bike&last_section_mode[]=walking&last_section_mode[]=bss&last_section_mode[]=bike` | walking |
| nop       | last_section_mode[]     | array of string   | Same as first_section_mode but for the last section  | walking     |
| nop       | language     | enum   | Language for path guidance in walking sections.<br>Enum values:<ul><li>de-DE</li><li>en-GB</li><li>en-US</li><li>es-ES</li><li>fr-FR</li>
<li>hi-IN</li><li>it-IT</li><li>ja-JP</li><li>nl-NL</li><li>pt-PT</li><li>ru-RU</li></ul>  | fr-FR     |
| nop       | depth                   | int               | Json response [depth](#depth)                        | 1           |

### Other parameters

| Required | Name            | Type    | Description                   | Default value |
|----------|-----------------|---------|-------------------------------|---------------|
| nop     | max_duration_to_pt   | int     | Maximum allowed duration to reach the public transport (same limit used before and after public transport).<br>Use this to limit the walking/biking part.<br>Unit is seconds | 30*60 s    |
| nop     | walking_speed        | float   | Walking speed for the fallback sections<br>Speed unit must be in meter/seconds         | 1.12 m/s<br>(4 km/h)<br>*Yes, man, they got the metric system* |
| nop     | bike_speed           | float   | Biking speed for the fallback<br>Speed unit must be in meter/seconds | 4.1 m/s<br>(14.7 km/h)   |
| nop     | bss_speed            | float   | Speed while using a bike from a bike sharing system for the fallback sections<br>Speed unit must be in meter/seconds | 4.1 m/s<br>(14.7 km/h)    |
| nop     | car_speed            | float   | Driving speed for the fallback sections<br>Speed unit must be in meter/seconds         | 16.8 m/s<br>(60 km/h)   |
| nop     | min_nb_journeys      | non-negative int | Minimum number of different suggested journeys<br>More in multiple_journeys  |             |
| nop     | max_nb_journeys      | positive int | Maximum number of different suggested journeys<br>More in multiple_journeys  |             |
| nop     | count                | int     | Fixed number of different journeys<br>More in multiple_journeys  |             |
| nop     | max_nb_transfers      | int     | Maximum number of transfers in each journey  | 10          |
| nop     | min_nb_transfers     | int     | Minimum number of transfers in each journey  | 0           |
| nop     | max_duration         | int     | If `datetime` represents the departure of the journeys requested, then the last public transport section of all journeys will end before `datetime` + `max_duration`.<br>If `datetime` represents the arrival of the journeys requested, then the first public transport section of all journeys will start after `datetime` - `max_duration`.<br>More useful when computing an isochrone (only `from` or `to` is provided)<br>Unit is seconds    | 86400       |
| nop     | wheelchair           | boolean | If true the traveler is considered to be using a wheelchair, thus only accessible public transport are used<br>be warned: many data are currently too faint to provide acceptable answers with this parameter on       | False       |
| nop     | direct_path          | enum    | Specify if Navitia should suggest direct paths (= only fallback modes are used).<br>Possible values: <ul><li>`indifferent`</li><li>`none` for only journeys using some PT</li><li>`only` for only journeys without PT</li><li>`only_with_alternatives` for different journey alternatives without PT</li> </ul>      | indifferent |
| nop     | direct_path_mode[]	     | array of strings     | Force direct-path modes. If this list is not empty, we only compute direct_path for modes in this list and filter all the direct_paths of modes in first_section_mode[]. It can take the following values: `walking`, `car`, `bike`, `bss`, `ridesharing`, `taxi`. It's an array, you can give multiple modes. If this list is empty, we will compute direct_path for modes of the first_section_modes.  | first_section_modes[]           |
| nop     | add_poi_infos[]      | boolean | Activate the output of additional infomations about the poi. For example, parking availability(BSS, car parking etc.) in the pois of response. Possible values are `bss_stands`, `car_park`    | []
| nop     | debug                | boolean | Debug mode<br>No journeys are filtered in this mode     | False       |
| nop     | free_radius_from     | int     | Radius length (in meters) around the coordinates of departure in which the stop points are considered free to go (crowfly=0) | 0           |
| nop     | free_radius_to	     | int     | Radius length (in meters) around the coordinates of arrival in which the stop points are considered free to go (crowfly=0)  | 0           |
| nop     | timeframe_duration	     | int     | Minimum timeframe to search journeys (in seconds, maximum allowed value = 86400). For example 'timeframe_duration=3600' will search for all interesting journeys departing within the next hour.  | 0           |

### Precisions on `forbidden_uris[]` and `allowed_id[]`

These parameters are filtering the vehicle journeys and the stop points used to compute the journeys.
`allowed_id[]` is used to allow *only* certain route options by *excluding* all others.
`forbidden_uris[]` is used to *exclude* specific route options.

Examples:

* A user doesn't like line A metro in hers city. She adds the parameter `forbidden_uris[]=line:A` when calling the API.
* A user would only like to use Buses and Tramways. She adds the parameter `allowed_id[]=physical_mode:Bus&allowed_id[]=physical_mode:Tramway`.

#### Technically

The journeys can only use allowed vehicle journeys (as present in the `public_transport` or `on_demand_transport` sections).
They also can only use the allowed stop points for getting in or out of a vehicle (as present in the `street_network`, `transfer` and `crow_fly` sections).

To filter vehicle journeys, the identifier of a line, route, commercial mode, physical mode or network can be used.

For filtering stop points, the identifier of a stop point or stop area can be used.

The principle is to create a blacklist using those 2 parameters:

* `forbidden_uris[]` adds the corresponding vehicle journeys (or stop points) to the blacklist of vehicle journeys (resp. stop_points).

* `allowed_id[]` works in 2 parts:

    * If an id related to a stop point is given, only the corresponding stop points are allowed (practically, all other are blacklisted). Else, all the stop points are allowed.
    * If an id related to a vehicle journey is given, only the corresponding vehicle journeys are allowed (practically, all other are blacklisted). Else, all the vehicle journeys are allowed.

The blacklisting constraints of `forbidden_uris[]` and `allowed_id[]` are combined. For example, if you give `allowed_id[]=network:SN&forbidden_uris[]=line:A`, only the vehicle journeys of the network SN that are not from the line A can be used to compute the journeys.

Let's illustrate all of that with an example.

![example](forbidden_example.png)

We want to go from stop A to stop B. Lines 1 and 2 can go from stop A to B. There is another stop C connected to A with lines 3 and 4, and connected to B with lines 5 and 6.

Without any constraint, all these objects can be used to propose a solution. Let's study some examples:

| `forbidden_uris[]` | `allowed_id[]`         | Result
|--------------------|------------------------|--------
| line 1, line 2     |                        | All the journeys will pass by stop C, using either of line 3, 4, 5 and 6
| stop A             |                        | No solution, as we can't get in any transport
| stop B             |                        | No solution, as we can't get out at destination
|                    | stop C                 | No solution, as we can't get in neither get out
| line 1, line 2     | line 3                 | No solution, as only line 3 can be taken
|                    | line 3, line 5         | All the journeys will pass by stop C using line 3 and 5
|                    | line 3, line 4, line5  | All the journeys will pass by stop C using (line 3 or 4) and line 5
|                    | line 3, line 5, stop C | No solution, as we can't get in neither get out
|                    | stop A, stop C, stop B | As without any constraint, passing via stop C is not needed
| stop A, stop B     | stop A, stop B         | No solution, as no stop point are allowed.

### Precisions on `free_radius_from/free_radius_to`

These parameters find the nearest stop point (within free_radius distance) to the given coordinates.
Then, it allows skipping walking sections between the point of departure/arrival and those nearest stop points.

Example:

![image](free_radius.png)

In this example, the stop points within the circle (SP1, SP2 et SP3) can be reached via a crowfly of 0 second. The other stop points, outside the circle, will be reached by walking.

<h3 id="journeys-objects">Objects</h3>

Here is a typical journey, all sections are detailed below

![image](typical_itinerary.png)

#### Main response

|Field|Type|Description
|-----|----|-----------
|journeys|array of [journeys](#journey)|List of computed journeys
|links|array of [link](#link)|Links related to the journeys <ul><li>`next`: search link with `&datetime = departure datetime of first journey + 1 second` and `&datetime_represents=departure` </li><li>`prev`: search link with `&datetime = arrival datetime of first journey - 1 second` and `&datetime_represents=arrival` </li><li> `first`: search link with `&datetime = departure date of first journey with 0 time part` and `&datetime_represents=departure` </li><li>`last`: search link with `&datetime = arrival date of last journey with 232359 time part` and `&datetime_represents=arrival` </li><li>`physical_modes`: physical_modes </li><li>and others: `physical_modes, pois lines, stop_areas, stop_points, poi_types, commercial_modes, addresses, networks, vehicle_journeys, routes` </li></ul>

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
  links               | [link](#link)                | Links related to this journey <ul><li>`same_journey_schedules`: search link for same journey schedules between two stop_areas using the same combination of public transport </li><li>`this_journey`: search link which returns the same journey </li></ul>
  type                | *enum* string                | Used to qualify a journey. See the [journey-qualification](#journey-qualification-process) section for more information
  fare                | [fare](#fare)                | Fare of the journey (tickets and price)
  tags                | array of string              | List of tags on the journey. The tags add additional information on the journey beside the journey type. See for example [multiple_journeys](#multiple-journeys).
  status              | *enum*                       | Status of the whole journey taking into acount the actual effect of disruptions retrieved on pick-ups and drop-offs used. See the [journey-disruption](#journeys-disruptions) section for more information.

<aside class="notice">
    When used with just a "from" or a "to" parameter, it will not contain any sections.
</aside>

#### Section

Field                    | Type                                          | Description
-------------------------|-----------------------------------------------|------------
type                     | *enum* string                                 | Type of the section.<ul><li>`public_transport`: public transport section</li><li>`street_network`: street section</li><li>`waiting`: waiting section between transport</li><li><p>`stay_in`: this “stay in the vehicle” section occurs when the traveller has to stay in the vehicle when the bus change its routing. Here is an exemple for a journey from A to B: (lollipop line)</p><p>![image](stay_in.png)</p></li><li>`transfer`: transfert section</li><li><p>`crow_fly`: teleportation section, most of the time. Useful to make navitia idempotent when starting from or arriving to a city or a stop_area (“potato shaped” objects) in order to route to the nearest stop_point. Be careful: neither “path” nor “geojson” available in a crow_fly section.</p><p> Can also be used when no street_network data are available and not be considered as teleportation. The distance of such a crow_fly section will be a straight line between the point of departure and arrival (hence the name 'crow_fly'). The duration of the section will be calculated with the Manhattan distance of the section (distance x √2). In this case, “geojson” is available.</p><p>![image](crow_fly.png)</p></li><li>`on_demand_transport`: vehicle may not drive along: traveler will have to call agency to confirm journey</li><li>`bss_rent`: taking a bike from a bike sharing system (bss)</li><li>`bss_put_back`: putting back a bike from a bike sharing system (bss)</li><li>`boarding`: boarding on vehicle (boat, on-demand-transport, plane, ...)</li><li>`alighting`: getting off a vehicle</li><li>`park`: parking a car</li><li>`ridesharing`: car-pooling section</li></ul>
id                       | string                                        | Id of the section
mode                     | *enum* string                                 | Mode of the street network and crow_fly: `Walking`, `Bike`, `Car`, 'Taxi'
duration                 | int                                           | Duration of this section
from                     | [places](#place)                              | Origin place of this section
to                       | [places](#place)                              | Destination place of this section
links                    | Array of [link](#link)                        | Links related to this section
display_informations     | [display_informations](#display-informations) | Useful information to display
additional_informations  | *enum* string                                 | Other information. It can be: <ul><li>`regular`: no on demand transport (odt)</li><li>`has_date_time_estimated`: section with at least one estimated date time</li><li>`odt_with_stop_time`: odt with fixed schedule, but travelers have to call agency!</li><li>`odt_with_stop_point`: odt where pickup or drop off are specific points</li><li>`odt_with_zone`: odt which is like a cab, from wherever you want to wherever you want, whenever it is possible</li></ul>
geojson                  | [GeoJson](https://www.geojson.org)             |
path                     | Array of [path](#path)                        | The path of this section
transfer_type            | *enum* string                                 | The type of this transfer it can be: `walking`, `stay_in`
stop_date_times          | Array of [stop_date_time](#stop-date-time)    | List of the stop times of this section
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
|value|string|cost: float formatted as string|
|currency|string|currency as specified in input data|

#### Ticket

|Field|Type|Description|
|-----|----|-----------|
|id|string|Id of the ticket|
|name|string|Name of the ticket|
|found|boolean|False if unknown ticket, True otherwise|
|cost|[cost](#cost)|Cost of the ticket|
|links|array of [link](#link)|Link to the [section](#section) using this ticket|

<h2 id="isochrones-api">Isochrones (currently in Beta)</h2>

>[Try a simple example on Navitia playground (click on "MAP" buttons for "wow effect")](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fisochrones%3Ffrom%3D2.377097%3B48.846905%26max_duration%3D2000%26min_duration%3D1000&token=3b036afe-0110-4202-b9ed-99718476c2e0)

>[Try a multi-color example on Navitia playground (click on "MAP" buttons for "WOW effect")](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fisochrones%3Ffrom%3D2.377097%253B48.846905%26boundary_duration%255B%255D%3D1000%26boundary_duration%255B%255D%3D2000%26boundary_duration%255B%255D%3D3000%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

``` shell
# Request
curl 'https://api.navitia.io/v1/coverage/sandbox/isochrones?from=stop_area:RAT:SA:GDLYO&max_duration=3600' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

# Response
HTTP/1.1 200 OK

{
    "isochrones":[
        {
            "geojson":{
                "type":"MultiPolygon",
                "coordinates":[
                    [
                        [
                            [
                                2.3186837324,
                                48.9324437042
                            ],
                            [
                                2.3187241561,
                                48.9324771012
                            ],
                            [
                                2.3190737256,
                                48.9327557777
                            ],
                            ["..."],
                            ["..."],
                            ["..."]
                        ]
                    ]
                ]
            }
        }
    ]
}
```

Also known as `/isochrones` service.

<aside class="warning">
    This service is under development. So it is accessible as a <b>"Beta" service</b>.
    <br>
    Every feed back is welcome on <a href="https://groups.google.com/forum/#!forum/navitia">https://groups.google.com/forum/#!forum/navitia</a>!
</aside>

This service gives you a multi-polygon response which
represents a same duration travel zone at a given time: https://en.wikipedia.org/wiki/Isochrone_map

As you can find isochrone tables using `/journeys`, this service is only another representation
of the same data, map oriented.

It is also really usefull to make filters on geocoded objects in order to find which ones are reachable at a given time within a specific duration.
You just have to verify that the coordinates of the geocoded object are inside the multi-polygon.

### Accesses

| url | Result |
|------------------------------------------|-------------------------------------|
| `/isochrones`                          | List of multi-polygons representing one isochrone step. Access from wherever land |
| `/coverage/{region_id}/isochrones` | List of multi-polygons representing one isochrone step. Access from on a specific coverage |

### <a name="isochrones-parameters"></a>Main parameters

<aside class="success">
    'from' and 'to' parameters works as exclusive parameters:
    <li>When 'from' is provided, 'to' is ignored and Navitia computes a "departure after" isochrone.
    <li>When 'from' is not provided, 'to' is required and Navitia computes a "arrival after" isochrone.
</aside>

<aside class="notice">
    Isochrones are time dependant. The duration boundary is actually an arrival time boundary.
    'datetime' parameter is therefore important: The result is not the same when computing
    an isochrone at 3am (few transports) or at 6pm (rush hour).
</aside>

| Required  | Name                    | Type          | Description                                                                               | Default value |
|-----------|-------------------------|---------------|-------------------------------------------------------------------------------------------|---------------|
| nop       | from                    | id            | The id of the departure of your journey. Required to compute isochrones "departure after" |               |
| nop       | to                      | id            | The id of the arrival of your journey. Required to compute isochrones "arrival before"    |               |
| yep       | datetime                | [iso-date-time](#iso-date-time) | Date and time to go                                                                          |               |
| yep       | boundary_duration[]  | int   | A duration delineating a reachable area (in seconds). Using multiple boundary makes map more readable |      |
| nop       | forbidden_uris[]        | id            | If you want to avoid lines, modes, networks, etc.</br> Note: the forbidden_uris[] concern only the public transport objects. You can't for example forbid the use of the bike with them, you have to set the fallback modes for this (`first_section_mode[]` and `last_section_mode[]`)                                                 |               |
| nop       | first_section_mode[]    | array of string   | Force the first section mode if the first section is not a public transport one. It takes one the following values: `walking`, `car`, `bike`, `bss`.<br>`bss` stands for bike sharing system.<br>It's an array, you can give multiple modes.<br><br>Note: choosing `bss` implicitly allows the `walking` mode since you might have to walk to the bss station.<br> Note 2: The parameter is inclusive, not exclusive, so if you want to forbid a mode, you need to add all the other modes.<br> Eg: If you never want to use a `car`, you need: `first_section_mode[]=walking&first_section_mode[]=bss&first_section_mode[]=bike&last_section_mode[]=walking&last_section_mode[]=bss&last_section_mode[]=bike` | walking |
| nop       | last_section_mode[]     | array of string   | Same as first_section_mode but for the last section  | walking     |

### Other parameters

| Required  | Name                 | Type  | Description                                                  | Default value |
|-----------|----------------------|-------|--------------------------------------------------------------|---------------|
| nop       | min_duration         | int   | Minimum duration delineating the reachable area (in seconds) |               |
| nop       | max_duration         | int   | Maximum duration delineating the reachable area (in seconds) |               |


### Tips

#### Understand the resulting isochrone

The principle of isochrones is to work like journeys.
So if one doesn't understand why a place is inside or outside an isochrone,
please compute a journey from the "center" of isochrone to that precise place.

To do that, just 3 changes are needed:

- provide a starting `datetime=` to compare arrival time evenly
- change endpoint: `/isochrones` to `/journeys`
- provide a destination using `&to=<my_place>`

Please remember that isochrones use crowfly at the end so they are less precise than journeys.

#### Isochrones without public transport

The main goal of Navitia is to handle public transport, so it's not recommended to avoid them.<br>However if your are willing to do that, you can use a little trick and
pass the parameters `&allowed_id=physical_mode:Bus&forbidden_id=physical_mode:Bus`.
You will only get circles.

#### Car isochrones

Using car in Navitia isochrones is not recommended.<br>It is only handled for compatibility with `/journeys` but tends to squash every other result.

<h2 id="route-schedules">Route Schedules</h2>

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/route_schedules' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 200 OK

{
    "pagination": {},
    "links": [],
    "disruptions": [],
    "notes": [],
    "feed_publishers": [],
    "exceptions": [],
    "route_schedules": [
    {
        "display_informations": {
            "direction": "Château de Vincennes (Saint-Mandé)",
            "code": "1",
            "network": "RATP",
            "links": [],
            "color": "F2C931",
            "commercial_mode": "Metro",
            "text_color": "000000",
            "label": "1"
        },
        "table": {
            "headers": [{
                    "display_informations": {
                        "direction": "Château de Vincennes (Saint-Mandé)",
                        "code": "",
                        "description": "",
                        "links": [],
                        "color": "",
                        "physical_mode": "Métro",
                        "headsign": "Château de Vincennes",
                        "commercial_mode": "",
                        "equipments": [],
                        "text_color": "",
                        "network": ""
                    },
                    "additional_informations": ["regular"],
                    "links": [{
                        "type": "vehicle_journey",
                        "id": "vehicle_journey:RAT:RATRM1REGA9828-1_dst_2"
                    }, {
                        "type": "physical_mode",
                        "id": "physical_mode:Metro"
                    }]
                },
                { ... },
                { ... }
            ],
            "rows": [{
                "stop_point": {
                    "codes": [ ... ],
                    "name": "La Défense Grande Arche",
                    "links": [],
                    "physical_modes": [{
                        "name": "Métro",
                        "id": "physical_mode:Metro"
                    }],
                    "coord": {"lat": "48.891935","lon": "2.237883"},
                    "label": "La Défense Grande Arche (Puteaux)",
                    "equipments": [],
                    "commercial_modes": [...],
                    "administrative_regions": [ ... ],
                    "id": "stop_point:RAT:SP:DENFE2",
                    "stop_area": { ... }
                },
                "date_times": [{
                    "date_time": "20160616T093300",
                    "additional_informations": [],
                    "links": [{
                        "type": "vehicle_journey",
                        "value": "vehicle_journey:RAT:RATRM1REGA9828-1_dst_2",
                        "rel": "vehicle_journeys",
                        "id": "vehicle_journey:RAT:RATRM1REGA9828-1_dst_2"
                    }],
                    "data_freshness": "base_schedule"
                }, {
                    "date_time": "20160617T094400",
                    "additional_informations": [],
                    "links": [{
                        "type": "vehicle_journey",
                        "value": "vehicle_journey:RAT:RATRM1REGA9827-1_dst_2",
                        "rel": "vehicle_journeys",
                        "id": "vehicle_journey:RAT:RATRM1REGA9827-1_dst_2"
                    }],
                    "data_freshness": "base_schedule"
                }]
            }]
        },
        "additional_informations": null,
        "links": [],
        "geojson": {}
    }]
}
```

Also known as `/route_schedules` service.

This endpoint gives you access to schedules of routes (so a kind of time table), with a response made
of an array of [route_schedule](#route-schedule), and another one of [note](#note). You can
access it via that kind of url: <https://api.navitia.io/v1/{a_path_to_a_resource}/route_schedules>

### Accesses

| url                                                     | Result                                                                                          |
|---------------------------------------------------------|-------------------------------------------------------------------------------------------------|
| `/coverage/{region_id}/{resource_path}/route_schedules` | List of the entire route schedules for a given resource                                         |
| `/coverage/{lon;lat}/coords/{lon;lat}/route_schedules`  | List of the entire route schedules for coordinates, navitia guesses the region from coordinates |

### Parameters

Required | Name               | Type      | Description                                                                                                                | Default Value
---------|--------------------|-----------|----------------------------------------------------------------------------------------------------------------------------|--------------
nop      | from_datetime      | [iso-date-time](#iso-date-time) | The date_time from which you want the schedules                                                      | the current datetime
nop      | duration           | int       | Maximum duration in seconds between from_datetime and the retrieved datetimes.                                             | 86400
nop      | depth              | int       | Json response [depth](#depth)                                                                                              | 1
nop      | items_per_schedule | int       | Maximum number of columns per schedule.                                                                                    |
nop      | forbidden_uris[]   | id        | If you want to avoid lines, modes, networks, etc.                                                                          |
nop      | data_freshness     | enum      | Define the freshness of data to use<br><ul><li>realtime</li><li>base_schedule</li></ul>                                    | base_schedule
nop      | disable_geojson    | boolean   | remove geojson fields from the response                                                                                    | False
nop      | direction_type     | enum      | Allow to filter the response with the route direction type property <ul><li>all</li><li>forward</li><li>backward</li></ul>Note: forward is equivalent to clockwise and inbound. When you select forward, you filter with: [forward, clockwise, inbound].<br>backward is equivalent to anticlockwise and outbound. when you select backward, you filter with: [backward, anticlockwise, outbound] | all

<h3>Objects</h3>

<h4 id="route-schedule">route_schedule object</h4>

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
additional_informations  | Array of String                               | Other information: TODO enum
display_informations     | [display_informations](#display-informations) | Usefull information about the the vehicle journey to display
links                    | Array of [link](#link)                        | Links to [line](#line), vehicle_journey, [route](#route), [commercial_mode](#commercial-mode), [physical_mode](#physical-mode), [network](#network)

#### row

Field      | Type                             | Description
-----------|----------------------------------|--------------------------
date_times | Array of [pt-date-time](#pt-date-time) | Array of public transport formated date time
stop_point | [stop_point](#stop-point)              | The stop point of the row

<h2 id="stop-schedules">Stop Schedules</h2>

>[Try it on Navitia playground (click on "EXT" buttons to see times)](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%253ARAT%253ASA%253AGDLYO%2Fstop_schedules%3Fitems_per_schedule%3D2%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/stop_schedules' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 200 OK
{
    "stop_schedules": [
        {
            "stop_point": {...},
            "links": [...],
            "date_times": [
                {
                    "date_time": "20160615T115300",
                    "additional_informations": [],
                    "links": [
                        {
                            "type": "vehicle_journey",
                            "value": "vehicle_journey:RAT:RATRM1REGA9869-1_dst_2",
                            "rel": "vehicle_journeys",
                            "id": "vehicle_journey:RAT:RATRM1REGA9869-1_dst_2"
                        }
                    ],
                    "data_freshness": "base_schedule"
                },
                {
                    "date_time": "20160616T115000",
                    "additional_informations": [],
                    "links": [
                        {
                            "type": "vehicle_journey",
                            "value": "vehicle_journey:RAT:RATRM1REGA9868-1_dst_2",
                            "rel": "vehicle_journeys",
                            "id": "vehicle_journey:RAT:RATRM1REGA9868-1_dst_2"
                        }
                    ],
                    "data_freshness": "base_schedule"
                },
                "..."
            ],
            "route": {...},
            "additional_informations": null,
            "display_informations": {
                "direction": "Château de Vincennes (Saint-Mandé)",
                "code": "1",
                "network": "RATP",
                "links": [],
                "color": "F2C931",
                "commercial_mode": "Metro",
                "text_color": "000000",
                "label": "1"
            }
        }
    ],
    "pagination": {...},
    "links": [...],
    "disruptions": [],
    "notes": [],
    "feed_publishers": [...],
    "exceptions": []
}
```

Also known as `/stop_schedules` service.

This endpoint gives you access to time tables going through a stop
point as:
![stop_schedules](https://upload.wikimedia.org/wikipedia/commons/thumb/7/7d/Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg/640px-Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg)

The response is made of an array of [stop_schedule](#stop-schedule), and another one of [note](#note).<br>[Context](#context) object provides the `current_datetime`, useful to compute waiting time when requesting Navitia without a `from_datetime`.<br>Can be accessed via: <https://api.navitia.io/v1/{a_path_to_a_resource}/stop_schedules>.

See how disruptions affect stop schedules in the [real time](#realtime) section.

### Accesses

| url | Result |
|--------------------------------------------------------|-----------------------------------------------------------------------------------|
| `/coverage/{region_id}/{resource_path}/stop_schedules` | List of the stop schedules grouped by `stop_point/route` for a given resource   |
| `/coverage/{lon;lat}/coords/{lon;lat}/stop_schedules`  | List of the stop schedules grouped by `stop_point/route` for coordinates, navitia guesses the region from coordinates |

### Parameters

Required | Name               | Type                            | Description                                                                                                                | Default Value
---------|--------------------|---------------------------------|----------------------------------------------------------------------------------------------------------------------------|--------------
nop      | from_datetime      | [iso-date-time](#iso-date-time) | The date_time from which you want the schedules                                                                            | the current datetime
nop      | duration           | int                             | Maximum duration in seconds between from_datetime and the retrieved datetimes.                                             | 86400
nop      | depth              | int                             | Json response [depth](#depth)                                                                                              | 1
nop      | forbidden_uris[]   | id                              | If you want to avoid lines, modes, networks, etc.                                                                          |
nop      | items_per_schedule | int                             | Maximum number of datetimes per schedule.                                                                                  |
nop      | data_freshness     | enum                            | Define the freshness of data to use to compute journeys <ul><li>realtime</li><li>base_schedule</li></ul>                   | realtime
nop      | disable_geojson    | boolean                         | remove geojson fields from the response                                                                                    | False
nop      | direction_type     | enum                            | Allow to filter the response with the route direction type property <ul><li>all</li><li>forward</li><li>backward</li></ul>Note: forward is equivalent to clockwise and inbound. When you select forward, you filter with: [forward, clockwise, inbound].<br>backward is equivalent to anticlockwise and outbound. When you select backward, you filter with: [backward, anticlockwise, outbound] | all

### <a name="stop-schedule"></a>Stop_schedule object

|Field|Type|Description|
|-----|----|-----------|
|display_informations|[display_informations](#display-informations)|Usefull information about the route to display|
|route|[route](#route)|The route of the schedule|
|date_times|Array of [pt-date-time](#pt-date-time)|When does a bus stops at the stop point|
|stop_point|[stop_point](#stop-point)|The stop point of the schedule|
|additional_informations|[additional_informations](#additional-informations)|Other informations, when no departures, in order of dominance<br> enum values:<ul><li>date_out_of_bounds: dataset loaded in Navitia doesn't cover this date</li><li>no_departure_this_day: there is no departure during the date/duration (for example, you have requested timetables for a sunday)</li><li>no_active_circulation_this_day: there is no more journeys for the date (for example you're too late, the line has closed for today)</li><li>terminus: there will never be departure, you're at the terminus of the line</li><li>partial_terminus: same as terminus, but be careful, some vehicles are departing from the stop some other days</li><li>active_disruption: no departure, due to a disruption</li></ul>|

<h2 id="terminus-schedules">Terminus Schedules</h2>

>[Try it on Navitia playground (click on "EXT" buttons to see times)](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%253ARAT%253ASA%253AGDLYO%2Fterminus_schedules%3Fitems_per_schedule%3D2%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/terminus_schedules' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
Same as stop_schedule but objects are embedded in the `terminus_schedules` section instead

HTTP/1.1 200 OK
{
    "terminus_schedules": [],
    "pagination": {...},
    "links": [...],
    "disruptions": [],
    "notes": [],
    "feed_publishers": [...],
    "exceptions": []
}
```

Also known as `/terminus_schedules` service.

This endpoint gives you access to time tables going through a stop point.
Departures are grouped observing all served stations after considered stop point. This can also be same as:<br>
![terminus_schedules](https://upload.wikimedia.org/wikipedia/commons/thumb/7/7d/Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg/640px-Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg)

The response is made of an array of [terminus_schedule](#terminus-schedule), and another one of [note](#note).<br>[Context](#context) object provides the `current_datetime`, useful to compute waiting time when requesting Navitia without a `from_datetime`.<br>Can be accessed via: <https://api.navitia.io/v1/{a_path_to_a_resource}/terminus_schedules>

### Accesses

| url | Result |
|--------------------------------------------------------|-----------------------------------------------------------------------------------|
| `/coverage/{region_id}/{resource_path}/terminus_schedules` | List of the schedules grouped by observing all served stations after considered stop_point for a given resource   |
| `/coverage/{lon;lat}/coords/{lon;lat}/terminus_schedules`  | List of the schedules grouped by observing all served stations after considered stop_point for coordinates, navitia guesses the region from coordinates |

### Parameters
Same as stop_schedule parameters.

### <a name="terminus-schedule"></a>Terminus_schedule object

Same as stop_schedule object.

<h2 id="departures">Departures</h2>

>[Try it on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%253ARAT%253ASA%253AGDLYO%2Fdepartures%3F&token=3b036afe-0110-4202-b9ed-99718476c2e0)

``` shell
#Request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/departures?from_datetime=20160615T1337' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#Response
HTTP/1.1 200 OK
{
   "departures":[
        {
            "display_informations":{
                "direction":"Ch\u00e2teau de Vincennes (Saint-Mand\u00e9)",
                "color":"F2C931",
                "physical_mode":"M?tro",
                "headsign":"Ch\u00e2teau de Vincennes",
                "commercial_mode":"Metro",
                "network":"RATP",
                "..."
            },
            "stop_point":{
                "name":"Esplanade de la D\u00e9fense",
                "physical_modes":[
                   {
                      "name":"M?tro",
                      "id":"physical_mode:Metro"
                   }
                ],
                "coord":{
                   "lat":"48.887843",
                   "lon":"2.250442"
                },
                "label":"Esplanade de la D\u00e9fense (Puteaux)",
                "id":"stop_point:RAT:SP:ESDEN2",
                "..."
            },
            "route":{
                "id":"route:RAT:M1_R",
                "name":"Ch\u00e2teau de Vincennes - La D\u00e9fense",
                "..."
            },
            "stop_date_time":{
                "arrival_date_time":"20160615T133700",
                "departure_date_time":"20160615T133700",
                "base_arrival_date_time":"20160615T133700",
                "base_departure_date_time":"20160615T133700"
            }
        },
        {"...":"..."},
        {"...":"..."},
        {"...":"..."}
   ]
}
```

Also known as `/departures` service.

This endpoint retrieves a list of departures from a specific datetime of a selected
object.
[Context](#context) object provides the `current_datetime`, useful to compute waiting time when requesting Navitia without a `from_datetime`.
Departures are ordered chronologically in ascending order as:
![departures](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/Display_at_bus_stop_sign_in_Karlovo_n%C3%A1m%C4%9Bst%C3%AD%2C_T%C5%99eb%C3%AD%C4%8D%2C_T%C5%99eb%C3%AD%C4%8D_District.JPG/640px-Display_at_bus_stop_sign_in_Karlovo_n%C3%A1m%C4%9Bst%C3%AD%2C_T%C5%99eb%C3%AD%C4%8D%2C_T%C5%99eb%C3%AD%C4%8D_District.JPG)

See how disruptions affect the next departures in the [real time](#realtime) section.

### Accesses

| url | Result |
|----------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `/coverage/{region_id}/{resource_path}/departures` | List of the next departures, multi-route oriented, only time sorted (no grouped by `stop_point/route` here) |
| `/coverage/{lon;lat}/coords/{lon;lat}/departures`  | List of the next departures, multi-route oriented, only time sorted (no grouped by `stop_point/route` here), navitia guesses the region from coordinates |

### Parameters

Required | Name             | Type                            | Description                                                                                                                           | Default Value
---------|------------------|---------------------------------|---------------------------------------------------------------------------------------------------------------------------------------|--------------
nop      | from_datetime    | [iso-date-time](#iso-date-time) | The date_time from which you want the schedules                                                                                       | the current datetime
nop      | duration         | int                             | Maximum duration in seconds between from_datetime and the retrieved datetimes.                                                        | 86400
nop      | count            | int                             | Maximum number of results.                                                                                                            | 10
nop      | depth            | int                             | Json response [depth](#depth)                                                                                                         | 1
nop      | forbidden_uris[] | id                              | If you want to avoid lines, modes, networks, etc.                                                                                     |
nop      | data_freshness   | enum                            | Define the freshness of data to use to compute journeys <ul><li>realtime</li><li>base_schedule</li></ul>                              | realtime
nop      | disable_geojson  | boolean                         | remove geojson fields from the response                                                                                               | false
nop      | direction_type   | enum                            | Allow to filter the response with the route direction type property <ul><li>all</li><li>forward</li><li>backward</li></ul>Note: forward is equivalent to clockwise and inbound. When you select forward, you filter with: [forward, clockwise, inbound].<br>backward is equivalent to anticlockwise and outbound. When you select backward, you filter with: [backward, anticlockwise, outbound] | all

### Departure objects

|Field|Type|Description|
|-----|----|-----------|
|route|[route](#route)|The route of the schedule|
|stop_date_time|Array of [stop_date_time](#stop-date-time)|Occurs when a bus does a stopover at the stop point|
|stop_point|[stop_point](#stop-point)|The stop point of the schedule|

<h2 id="arrivals">Arrivals</h2>

>[Try it on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%253ARAT%253ASA%253AGDLYO%2Farrivals%3F&token=3b036afe-0110-4202-b9ed-99718476c2e0)

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
|--------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `/coverage/{region_id}/{resource_path}/arrivals` | List of the arrivals, multi-route oriented, only time sorted (no grouped by `stop_point/route` here)        |
| `/coverage/{lon;lat}/coords/{lon;lat}/arrivals`  | List of the arrivals, multi-route oriented, only time sorted (no grouped by `stop_point/route` here), navitia guesses the region from coordinates  |

### Parameters

they are exactly the same as [departures](#departures).

<h2 id="line-reports">Line reports</h2>

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/line_reports' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response, composed by 2 main lists: "line_reports" and "disruptions"
HTTP/1.1 200 OK

{
"disruptions": [
        #list of linked disruptions
],
"line_reports": [
    {
        "line": {
            #main object (line) and links within its own disruptions
        }
        "pt_objects": [
            #list of all disrupted objects related to the line: stop_area, networks, etc...
        ]
    },
    {
        #Another line with its objects
    }
]
}
```

This service provides the state of public transport traffic, grouped by lines and all their stops.<br>It can be called for an overall coverage or for a specific object.<br>Can be accessed via: <https://api.navitia.io/v1/{a_path_to_a_resource}/line_reports>.

<img src="./images/traffic_reports.png" alt="Traffic reports" width="300"/>

### Parameters

For example:

-   overall public transport line report on Ile de France coverage
    -   <https://api.navitia.io/v1/coverage/fr-idf/line_reports>
-   Is there any perturbations on the RER network?
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/line_reports>
-   Is there any perturbations on the "RER A" line?
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/lines/line:TRN:DUA810801043/line_reports>

Required | Name             | Type                            | Description                                       | Default Value
---------|------------------|---------------------------------|---------------------------------------------------|--------------
no       | since            | [iso-date-time](#iso-date-time) | Only display active disruptions after this date   |
no       | until            | [iso-date-time](#iso-date-time) | Only display active disruptions before this date  |
no       | count            | int                             | Maximum number of results.                        | 25
no       | depth            | int                             | Json response [depth](#depth)                     | 1
no       | forbidden_uris[] | id                              | If you want to avoid lines, modes, networks, etc. |
no       | disable_geojson  | boolean                         | remove geojson fields from the response           | false

The response is made of an array of [line_reports](#line-reports),
and another one of [disruptions](#disruption).

There are inner links between this 2 arrays:
see the [inner-reference](#inner-references) section to use them.

### Line report object

``` shell
#links between objects in a line_reports response
{
  "disruptions": [
    {
      "status": "active",
      "id": "17283fae-7dcf-11e8-898e-005056a47b86"
    },
    {
      "status": "active",
      "id": "140a9970-0c9b-11e8-b2b6-005056a44da2"
    }
  ],
  "line_reports": [
    {
      "line": {
        "links": [],
        "id": "line:1"
      },
      "pt_objects": [
        {
          "embedded_type": "stop_point",
          "stop_point": {
            "name": "SP 1",
            "links": [
              {
                "internal": true,
                "type": "disruption",
                "id": "140a9970-0c9b-11e8-b2b6-005056a44da2",
                "rel": "disruptions",
                "templated": false
              }
            ],
          "id": "stop_point:1"
          }
        }
      ]
    },
    {
    "line": {
        "id": "line:CAE:218",
        "links": [
              {
                "internal": true,
                "type": "disruption",
                "id": "17283fae-7dcf-11e8-898e-005056a47b86",
                "rel": "disruptions",
                "templated": false
              }
        ]
    },
    "pt_objects": [
        {
            "embedded_type": "line",
            "line": {
                "id": "line:CAE:218",
                "links": [
                    {
                        "internal": true,
                        "type": "disruption",
                        "id": "17283fae-7dcf-11e8-898e-005056a47b86",
                        "rel": "disruptions",
                        "templated": false
                    }
                ]
            }
        }
    ]
}
]
}
```
Line_reports is an array of some line_report object.

One Line_report object is a complex object, made of a line, and an array
of [pt_objects](#pt-objects) linked (for example stop_areas, stop_point or network).

#### What a **complete** response **means**

-   multiple line_reports
    -   line 1
          -   stop area concorde > internal link to disruption "green"
          -   stop area bastille > internal link to disruption "pink"
    -   line 2 > internal link to disruption "blue"
          -   network RATP > internal link to disruption "green"
          -   line 2 > internal link to disruption "blue"
    -   line 3 > internal link to disruption "yellow"
          -   stop point bourse > internal link to disruption "yellow"
-   multiple disruptions (disruption target links)
    -   disruption "green"
    -   disruption "pink"
    -   disruption "blue"
    -   disruption "yellow"
    -   Each disruption contains the messages to show.

Details for disruption objects: [disruptions](#disruptions)

#### What a line_report object **contains**

-   1 line which is the grouping object
    -   it can contain links to its disruptions.<br>These disruptions are globals and might not be applied on stop_areas and stop_points.
-   1..n pt_objects
    -   each one contains at least a link to its disruptions.

<h2 id="traffic-reports">Traffic reports</h2>

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/traffic_reports' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

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
"disruptions": [
        #list of linked disruptions
    ]
}
```

Also known as `/traffic_reports` service.

This service provides the state of public transport traffic, grouped by network.<br>It can be called for an overall coverage or for a specific object.<br>Can be accessed via: <https://api.navitia.io/v1/{a_path_to_a_resource}/traffic_reports>

### Parameters

For example:

-   overall public transport traffic report on Ile de France coverage
    -   <https://api.navitia.io/v1/coverage/fr-idf/traffic_reports>
-   Is there any perturbations on the RER network?
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/traffic_reports>
-   Is there any perturbations on the "RER A" line?
    -   <https://api.navitia.io/v1/coverage/fr-idf/networks/network:RER/lines/line:OIF:810:AOIF741/line_reports?>

Required | Name             | Type                            | Description                                         | Default Value
---------|------------------|---------------------------------|-----------------------------------------------------|--------------
no       | since            | [iso-date-time](#iso-date-time) | Only display active disruptions after this date     |
no       | until            | [iso-date-time](#iso-date-time) | Only display active disruptions before this date    |
no       | count            | int                             | Maximum number of results.                          | 10
no       | depth            | int                             | Json response [depth](#depth)                       | 1
no       | forbidden_uris[] | id                              | If you want to avoid lines, modes, networks, etc.   |
no       | disable_geojson  | boolean                         | remove geojson fields from the response             | false

The response is made of an array of [traffic_reports](#traffic-reports),
and another one of [disruptions](#disruption).

There are inner links between this 2 arrays:
see the [inner-reference](#inner-references) section to use them.


### Traffic report object

``` shell
#links between objects in a traffic_reports response
{
"traffic_reports": [
    {
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
        }
    ]
    },{
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
        }
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
        }
    ]
    }
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

#### What a **complete** response **means**

-   multiple traffic_reports
    -   network "bob"
          -   line "1" > internal link to disruption "green"
          -   line "12" > internal link to disruption "pink"
          -   stop_area "bobito" > internal link to disruption "red"
    -   network "bobette" > internal link to disruption "blue"
          -   line "A" > internal link to disruption "green"
          -   line "C" > internal link to disruption "yellow"
          -   stop_area "bobito" > internal link to disruption "red"
-   multiple disruptions (disruption target links)
    -   disruption "green"
    -   disruption "pink"
    -   disruption "red"
    -   disruption "blue"
    -   disruption "yellow"
    -   Each disruption contains the messages to show.

Details for disruption objects: [disruptions](#disruptions)

#### What a traffic_report object **contains**

-   1 network which is the grouping object
    -   it can contain links to its disruptions.<br>These disruptions are globals and might not be applied on lines or stop_areas.
-   0..n lines
    -   each line contains at least a link to its disruptions
-   0..n stop_areas
    -   each stop_area contains at least a link to its disruptions<br>If a stop_area is used by multiple networks, it will appear each time.


Equipment_Reports
---------------------------------------------

``` shell
#request
$ curl 'https://api.navitia.io/v1/coverage/<my_coverage>/equipment_reports'
```

``` shell
# response, composed by 1 main list: "equipment_reports"
HTTP/1.1 200 OK

{
    "equipment_reports": [
        {
            "line": {15 items},
            "stop_area_equipments": [
                {
                    "equipment_details": [
                        {
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
                                "updated_at": "2019-05-17T15:54:53+02:00"
                            }
                        "embedded_type": "escalator",
                        "id": "2702",
                        "name": "du quai direction Vaulx-en-Velin La Soie  jusqu'à la sortie B",
                        },
                    ]
                    "stop_area": {9 items},
                },
            ]
        },
    ],
}
```

Also known as the `"/equipment_reports"` service.

This service provides the state of equipments such as lifts or elevators that are giving you better accessibility to public transport facilities.<br>The endpoint will report accessible equipment per stop area and per line. Which means that an equipment detail is reported at the stop area level, with all stop areas gathered per line.<br>Some of the fields (cause, effect, periods etc...) are only displayed if a realtime equipment provider is setup with available data. Otherwise, only information provided by the NTFS will be reported.<br>For more information, refer to [Equipment reports](#equipment-reports) API description.<br>Can be accessed via: <https://api.navitia.io/v1/{a_path_to_a_resource}/equipment_reports>

<aside class="warning">
    This feature requires a specific configuration from a equipment service provider.
    Therefore this service is not available by default.
</aside>

### Parameters

Required | Name             | Type   | Description                                         | Default Value
---------|------------------|--------|-----------------------------------------------------|--------------
no       | count            | int    | Elements per page                                   | 10
no       | depth            | int    | Json response [depth](#depth)                       | 1
no       | filter           | string | A [filter](#filter) to refine your request          |
no       | forbidden_uris[] | id     | If you want to avoid lines, modes, networks, etc.   |
no       | start_page       | int    | The page number (cf. the [paging section](#paging)) | 0
