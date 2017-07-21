The line sections is a way to impact some routes between 2 stops areas

# Blocking a line section

For each vehicle journey corresponding to the line/route, we get the corresponding base vehicle journey and identify all the smallest sections starting with the first stop and ending with the other.  Then, the new vehicle journey doesn't stop at all corresponding stop points.

## Case of a lollipop vehicle journey

For example, if we want to disrupt the section B C on the following lollipop vehicle journey:

```
Vehicle does:
A - B - C - D - E - B - C - F

      E <---- D
      |       ^
      v       |
A --> B ====> C --> F
      [#######]
```

The disrupted vehicle journey will not stop at B and C:

```
    [###]           [###]
A --------- D - E --------- F
```

## Case of multiple stop_points from a same stop_area in a vehicle journey

### Consecutive stop_point

This is very specific (case of a bus C2 at Commerce in Nantes, France):
```
    +---------------------------+     +------------------------------------------------------------+
    |     stop_area:Cirque      |     |                    stop_area:Commerce                      |
    |                           |     |                                                            |
--- | --> stop_point:Cirque --- | --- | --> stop_point:Commerce1 --------> stop_point:Commerce2 ---|----->
    |                           |     |                                                            |
    +---------------------------+     +------------------------------------------------------------+

section disrupted for Cirque -> Commerce (and resulting vehicle journey):
          [####################################################]
-------------------------------------------------------------------------> stop_point:Commerce2 --------->

sections disrupted for Commerce -> Commerce (and resulting vehicle journey):
                                            [##################]           [##################]
--------> stop_point:Cirque ----------------------------------------------------------------------------->
```
The idea is the same, we take **all** the **shortest** sections matching the stop area provided.

If a line section disruption is created from `Cirque` (stop area) to `Commerce` (stop area),
then the section disrupted is `stop_point:Cirque -> stop_point:Commerce1`.

If a line section disruption is created from `Commerce` to `Commerce` (to disrupt only one stop),
then the sections disrupted are `stop_point:Commerce1 -> stop_point:Commerce1` and `stop_point:Commerce2 -> stop_point:Commerce2`.
So there is no way right now to disrupt only one of the stop_points.


### Lollipop on same stop_area (but different stop_points)

The behaviour is the same:
```
    +----------------+     +--------------------------------+
    |     s_a:A      |     |              s_a:B             |
    |                |     |                                |
--- | --> s_p:A1 --- | --- | --> s_p:B1           s_p:B2 ---|----->
    |                |     |       |                ^       |
    |                |     |       |                |       |
    +----------------+     +--------------------------------+
                                   |                |
                           +--------------------------------+
                           |       |      s_a:C     |       |
                           |       |                |       |
                           |       +---> s_p:C1 ----+       |
                           |                                |
                           +--------------------------------+

section disrupted for A -> B (and resulting vehicle journey):
           [##########################]
-----------------------------------+              s_p:B2 --------->
                                   |                ^
                                   |                |
                                   +---> s_p:C1 ----+

sections disrupted for B -> B (and resulting vehicle journey):
                                [#####]          [#####]
--------> s_p:A1 ------------------+                +------------->
                                   |                |
                                   +---> s_p:C1 ----+
```
If a line section disruption is created from `A` to `B`,
then the section disrupted is `s_p:A1 -> s_p:B1` (no other and no shorter section matches stop_areas)

If a line section disruption is created from `B` to `B`,
then the sections disrupted are `s_p:B1 -> s_p:B1` and `s_p:B2 -> s_p:B2`.


# Displaying line sections

The chosen approach is a bit like how delays are handled with `impacted_objects` being a `trip` and `impacted_stops` being the stop_times of this trip

We impact a line and add some precisions on how this line is impacted

```javascript
{
  "status":"active",
  // lots of additional fields
  "impacted_objects":[
  {
     "pt_object": {
        "embedded_type": "line",
        "line": {
          // a line
        }
        "id":"id_of_the_line",
     }, 
     "impacted_section":{ // <- the integration is a bit tricky, you need to check this field to know the impact is on a line section
        "from": {
            // a pt object
            "embedded_type":"stop_area",
            "stop_area":{
            }
        },
        "to": {
            // a pt object
            "embedded_type":"stop_area",
            "stop_area":{
            }
        },
        // no routes for the moment
      },
  }]
}
```

The downside is that the integration is harder, the user needs to check wheter or not a field `impacted_section` is present to treat the impacted line differently, 
but:
* it's like what we have for the delays on trips
* we find that it's quite clear
* it's fully retrocompatible (especially since line sections are currently displayed as line :wink:)
* no `/line_sections` api is needed and the line section objects are not displayed in the api

# when to display the impact on a line section

## pt ref

I think an example will be easier to understand. We impact the stops [C, D, E] of the 'route_1_of_the_line_1'

```
A           B           C           D           E           F
o --------- o --------- o --------- o --------- o --------- o   <- route_1_of_the_line_1

                        XXXXXXXXXXXXXXXXXXXXXXXXX               <- impact on the route_1_of_the_line_1 on [C, E]
```

### common cases

On ptref we can display the line section disruption on a ptref call if the last object of the ptref call is directly linked to an **active** disruption

We'll at least link the line section disruption to:
* the vehicle journeys of the impacted route
* the stop points in [from, to] for each impacted vehicle_journeys

examples:

this is the navitia call and YES means we display the disruption, NO means we do not display it, 
NOT_SPECIFIED means that we haven't yet decided whether or not they are to be displayed

`/routes/route_1_of_the_line_1` --> YES

`/stop_areas/A`  --> NO

`/stop_areas/B`  --> NO

`/stop_areas/C`  --> NOT_SPECIFIED

`/stop_areas/D`  --> NOT_SPECIFIED

`/stop_areas/E`  --> NOT_SPECIFIED

`/stop_areas/F`  --> NO

`/stop_points/A_1_of_route_1_of_line_1`  --> NO

`/stop_points/B_1_of_route_1_of_line_1`  --> NO

We display only the stoppoints impacted by the line section

`/stop_points/C_1_of_route_1_of_line_1`  --> YES

`/stop_points/C_2_of_route_2_of_line_1`  --> NO

`/stop_points/C_3_of_line_2`  --> NO


`/stop_points/D_1_of_route_1_of_line_1`  --> YES

`/stop_points/E_1_of_route_1_of_line_1`  --> YES

`/stop_points/F_1_of_route_1_of_line_1`  --> NO


`/routes/route_2_of_the_line_1`  --> NO

`/lines/line_1`  -->  NOT_SPECIFIED

`/lines/line_2`  -->  NO

`/vehicle_journeys/vj1_of_route_1_of_line_1`  --> YES


We'll not use the whole ptref query to filter the disruption displayed it can thus lead to some difficult to understand behaviour:

`/stop_points/A_1/routes/route_1_of_the_line_1`  --> YES

`/stop_points/C_1_of_an_impacted_route/routes/route_2_of_the_line_1`  --> NO

`/networks/network_of_the_route_1 `  --> NO

### ptref with /disruptions

This is for the calls api.navitia.io/v1/coverage/bob/<some pt ref filters>/disruptions

The /disruption api is meant to be a direct representation of the object model below so I think it's ok the keep the same mechanism as before 
where only the objects with disruptions technically linked to them have disruptions.


### ptref with /traffic_reports

This is for the calls api.navitia.io/v1/coverage/bob/<some pt ref filters>/traffic_reports

This /traffic_reports api is meant to be a clever representation of the disruptions active on the object and thus we can do more things on this

`/stop_areas/B/traffic_reports`  --> NO

`/stop_areas/C/traffic_reports`  --> YES

`/lines/line_1/traffic_reports`  --> YES

`/lines/line_2/traffic_reports`  --> NO

`/stop_areas/A/routes/route_1_of_the_line_1/traffic_reports`  --> YES (I think NO would be better it but this will be quite difficult for not that much gain)

`/stop_areas/C/routes/route_2_of_the_line_1/traffic_reports`  --> NO

`/networks/network_of_the_route_1/traffic_reports `  --> YES

`/traffic_reports` display explicitly only `networks`, `lines` and `stop_areas`

so for `routes`, `stop_points` and `vehicle_journeys` we display a line section disruption if we display a stop_areas impacted by the disruption

`/routes/route_1_of_the_line_1/traffic_reports`  --> YES

`/routes/route_2_of_the_line_1/traffic_reports`  --> YES because we display the disruption on a route_2_of_the_line_1's stop area

`/stop_points/C_1/traffic_reports`  --> YES since it's stop_area is part of an impacted line section

`/vehicle_journeys/vj1_of_route_1_of_line_1/traffic_reports`  --> YES


To sum up, `/disruptions` is a technical view whereas `/traffic_reports` should be user friendly thus:

`/networks/network_of_the_route_1/disruptions`  --> NO

`/networks/network_of_the_route_1/traffic_reports`  --> YES


## schedules

I don't think we need to do some stuff here, the same rules as the common cases can be used.

## journeys

If a journey use a VJ of an impacted route on at least one stop point impacted by the line section we display it.


# modelisation

technically we keep the LineSection as is:

```c++
struct LineSection {
    Line* line;
    StopArea* start_point;
    StopArea* end_point;
    std::vector<Route*> routes;
};
```

but a line section disruption will be stored in:
* each impacted vehicle_journeys of each routes
* each stoppoints in [start_point, end_point] for each vj for each routes
