<a name="timetables"></a>![Timetables](/images/line_schedules.png)Timetables
=================================

The Timetables features gives you access to line schedules on public transport, 
allowing you to find the times public transport is expected at specific stops.

It returns a variety of differents informations, including: stop schedules, origin and destination, 
timetables, and accessibility information.

In order to display line schedules, you may have to use these APIs (click on them for details):

-   **[Public transportation objects](#pt-ref)** : List of the public transport
    objects of a region

| url | Result |
|---------------------------------------------------------|-------------------------------------|
| `/coverage/{region_id}/{collection_name}`               | Collection of objects of a region   |
| `/coverage/{region_id}/{collection_name}/{object_id}`   | Information about a specific region |
| `/coverage/{lon;lat}/{collection_name}`                 | Collection of objects of a region   |
| `/coverage/{lon;lat}/{collection_name}/{object_id}`     | Information about a specific region |

-   **[Route Schedules](#route-schedules)** and **[Stop Schedules](#stop-schedules)** : 
Compute time tables for a given resource

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `coverage/{resource_path}/route_schedules` | List of the entire route schedules for a given resource                                                       |
| `coverage/{resource_path}/stop_schedules`  | List of the stop schedules grouped by ``stop_point/route`` for a given resource                               |

