<a name="ressources"></a>Ressources overview
==========================================

The only endpoint of this version of the api is :
<https://api.navitia.io/v1/>

All the resources return a response containing a links object, a paging
object, and the requested object.

-   **[Coverage](#coverage)** : List of the region covered by navitia

| url | Result |
|----------------------------------------------|-------------------------------------|
| `get` /coverage                              | List of the areas covered by navitia|
| `get` /coverage/*region_id*                  | Information about a specific region |
| `get` /coverage/*region_id*/coords/*lon;lat* | Information about a specific region |

-   **[Public transportation objects](#pt-ref)** : List of the public transport
    objects of a region

| url | Result |
|---------------------------------------------------------|-------------------------------------|
| `get` /coverage/*region_id*/*collection_name*           | Collection of objects of a region   |
| `get` /coverage/region_id/collection_name/object_id     | Information about a specific region |
| `get` /coverage/*lon;lat*/*collection_name*             | Collection of objects of a region   |
| `get` /coverage/*lon;lat*/*collection_name*/*object_id* | Information about a specific region |

-   **[Journeys](#journeys)** : Compute journeys

| url | Result |
|------------------------------------------|-------------------------------------|
| `get` /coverage/*resource_path*/journeys | List of journeys or isochrones      |
| `get` /journeys                          | List of journeys or isochrones      |


-   **[Route Schedules](#route-schedules)**, **[Stop Schedules](#stop-schedules)**, **[Departures](#departures)** and **[Arrivals](#arrivals)** : 
Compute time tables for a given resource

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `get` /coverage/*resource_path*/route_schedules | List of the entire route schedules for a given resource                                                       |
| `get` /coverage/*resource_path*/stop_schedules  | List of the stop schedules grouped by ``stop_point/route`` for a given resource                               |
| `get` /coverage/*resource_path*/departures      | List of the next departures, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here) |
| `get` /coverage/*resource_path*/arrivals        | List of the arrivals, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here)        |


-   **[Places](#places)** and **[PT_objects](#pt-objects)** : Search in the datas

| url | Result |
|------------------------------------------------|-------------------------------------|
| `get` /coverage/places                         | List of geographical objects        |
| `get` /coverage/pt_objects                     | List of public transport objects    |


-   **[Places nearby](#places-nearby)** : List of objects near an object or a coord

| url | Result |
|------------------------------------------------|-----------------------------------------------------------|
| `get` /coord/*lon;lat*/places_nearby           | List of objects near the resource without any region id   |
| `get` /coverage/*lon;lat*/places_nearby        | List of objects near the resource without any region id   |
| `get` /coverage/*resource_path*/places_nearby  | List of objects near the resource                         |


