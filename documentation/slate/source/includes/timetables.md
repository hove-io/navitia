<a name="timetables"></a>Timetables
=================================

-   **[Route Schedules](#route-schedules)** and **[Stop Schedules](#stop-schedules)** : 
Compute time tables for a given resource

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `get` /coverage/*resource_path*/route_schedules | List of the entire route schedules for a given resource                                                       |
| `get` /coverage/*resource_path*/stop_schedules  | List of the stop schedules grouped by ``stop_point/route`` for a given resource                               |

