<a name="nearby"></a>![PLaces nearby](/images/places_nearby.png)Places nearby
==================================

The Places Nearby feature displays the different transport options around a location - a GPS coordinate, 
or address, for example. This function provides information like: stops, lines, timetables, 
next arrivals, etc. 

Using OpenStreetMap data, this function also provides information about bicycle parking, public park schedules, and parking fees.

You can use these APIs (click on them for details):

-   **[Coverage](#coverage)** : List of the region covered by navitia

| url | Result |
|----------------------------------------------|-------------------------------------|
| `coverage`                              | List of the areas covered by navitia|
| `coverage/{region_id}`                  | Information about a specific region |
| `coverage/{region_id}/coords/{lon;lat}` | Information about a specific region |

-   **[Public transportation objects](#pt-ref)** : List of the public transport
    objects of a region

| url | Result |
|---------------------------------------------------------|-------------------------------------|
| `/coverage/{region_id}/{collection_name}`               | Collection of objects of a region   |
| `/coverage/{region_id}/{collection_name}/{object_id}`   | Information about a specific region |
| `/coverage/{lon;lat}/{collection_name}`                 | Collection of objects of a region   |
| `/coverage/{lon;lat}/{collection_name}/{object_id}`     | Information about a specific region |

-   **[Places nearby](#places-nearby)** : List of objects near an object or a coord

| url | Result |
|------------------------------------------------|-----------------------------------------------------------|
| `/coord/{lon;lat}/places_nearby`           | List of objects near the resource without any region id   |
| `/coverage/{lon;lat}/places_nearby`        | List of objects near the resource without any region id   |
| `/coverage/{resource_path}/places_nearby`  | List of objects near the resource                         |

-   **[Stop Schedules](#stop-schedules)**, **[Departures](#departures)** and **[Arrivals](#arrivals)** : 
Compute time tables for a given resource

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `/coverage/{resource_path}/stop_schedules`  | List of the stop schedules grouped by ``stop_point/route`` for a given resource                               |
| `/coverage/{lon;lat}/stop_schedules`        | List of the stop schedules grouped by ``stop_point/route`` for coordinates                                    |
| `/coverage/{resource_path}/departures`      | List of the next departures, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here) |
| `/coverage/{lon;lat}/departures`            | List of the next departures, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here) |
| `/coverage/{resource_path}/arrivals`        | List of the arrivals, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here)        |
| `/coverage/{lon;lat}/arrivals`              | List of the arrivals, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here)        |


