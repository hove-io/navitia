<a name="next_departures_and_arrivals"></a>![Departures](/images/departures.png)Next departures and arrivals
=======================================================================

The "Next Departures" feature provides the next scheduled departures or arrivals 
for a specific mode of public transport (bus, tram, metro, train) at your selected stop, near coordinates, etc. 

* use `stop_schedules` if you want to display departures grouped by route (2 next departures for each route for example)
![stop_schedules](https://upload.wikimedia.org/wikipedia/commons/thumb/7/7d/Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg/640px-Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg)

* use `departures` or `arrivals` if you want to display a multi-route table (like big departure boards in train stations)
![departures](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/Display_at_bus_stop_sign_in_Karlovo_n%C3%A1m%C4%9Bst%C3%AD%2C_T%C5%99eb%C3%AD%C4%8D%2C_T%C5%99eb%C3%AD%C4%8D_District.JPG/640px-Display_at_bus_stop_sign_in_Karlovo_n%C3%A1m%C4%9Bst%C3%AD%2C_T%C5%99eb%C3%AD%C4%8D%2C_T%C5%99eb%C3%AD%C4%8D_District.JPG)

In order to display departures, you may have to use these APIs (click on them for details):

-   **[Public transportation objects](#pt-ref)** : List of the public transport
    objects of a region

| url | Result |
|---------------------------------------------------------|-------------------------------------|
| `/coverage/{region_id}/{collection_name}`               | Collection of objects of a region   |
| `/coverage/{region_id}/{collection_name}/{object_id}`   | Information about a specific region |
| `/coverage/{lon;lat}/{collection_name}`                 | Collection of objects of a region   |
| `/coverage/{lon;lat}/{collection_name}/{object_id}`     | Information about a specific region |

-   **[Stop Schedules](#stop-schedules)**, **[Departures](#departures)** and **[Arrivals](#arrivals)** : 
Compute time tables for a given resource

| url | Result |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `/coverage/{resource_path}/stop_schedules`  | List of the stop schedules grouped by ``stop_point/route`` for a given resource                               |
| `/coverage/{resource_path}/departures`      | List of the next departures, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here) |
| `/coverage/{resource_path}/arrivals`        | List of the arrivals, multi-route oriented, only time sorted (no grouped by ``stop_point/route`` here)        |



