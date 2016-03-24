<a name="timetables"></a>Timetables
=================================

-   **[Route Schedules](#route-schedules)**, **[Stop Schedules](#stop-schedules)**, **[Departures](#departures)** and **[Arrivals](#arrivals)** : 
Compute time tables for a given resource

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `get` /coverage/*resource_path*/route_schedules | List of the entire route schedules for a given resource                                                       |
| `get` /coverage/*resource_path*/stop_schedules  | List of the stop schedules grouped by ``stop_point/route`` for a given resource                               |
| `get` /coverage/*resource_path*/departures      | List of the next departures, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here) |
| `get` /coverage/*resource_path*/arrivals        | List of the arrivals, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here)        |

