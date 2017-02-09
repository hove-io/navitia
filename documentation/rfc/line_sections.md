The line sections is a way to impact some routes between 2 stops areas

We know (thanks to @eturk!) how to handle blocking line section (the buses don't stop anymore at the stops between the 2 stops), now we need to kown how to show them

Here's a proposal on how to handle them:

# representation

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
     "line_section":{ // <- the integration is a bit tricky, you need to check this field to know the impact is on a line section
        "from": {
            // a stop area
        },
        "to": {
            // a stop area
        },
        // routes ?
      },
  }]
}
```

The downside is that the integration is harder, the user needs to check wheter or not a field `impacted_line_sections` is present to treat the impacted line differently, 
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

`/stop_points/E_1_of_route_1_of_line_1`  --> NO

`/stop_points/F_1_of_route_1_of_line_1`  --> NO


`/routes/route_2_of_the_line_1`  --> NO

`/lines/line_1`  -->  YES

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

`/lines/line_1/traffic_reports`  -->  YES

`/lines/line_2/traffic_reports`  -->  NO

`/routes/route_1_of_the_line_1/traffic_reports`  --> YES

`/routes/route_2_of_the_line_1/traffic_reports`  --> NO

`/stop_areas/A/routes/route_1_of_the_line_1/traffic_reports`  --> YES (I think NO would be better it but this will be quite difficult for not that much gain)

`/stop_areas/C/routes/route_2_of_the_line_1/traffic_reports`  --> NO

`/networks/network_of_the_route_1/traffic_reports `  --> YES

`/stop_points/C_1/traffic_reports`  --> YES since it's stop_area is part of an impacted line section

`/vehicle_journeys/vj1_of_route_1_of_line_1/traffic_reports`  --> NO (It would be better to display it, but we do not display other disruptions there, would it be worth it to change this ?)


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
* the line
* each vehicle_journeys of each routes
* each stoppoints in [start_point, end_point] for each vj for each routes
