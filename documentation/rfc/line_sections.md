The line sections is a way to impact some routes between 2 stops areas

We know (thanks to @eturk!) how to handle blocking line section (the buses don't stop anymore at the stops between the 2 stops), now we need to kown how to show them

Here's a proposal on how to handle them:

# representation

## chaos like

I think the easiest way to show them would be as a classic pt_object
```javascript
{
  "status":"active",
  // lots of additional fields
  "impacted_objects":[
    {
      "pt_object":{
        "embedded_type":"line_section", // <--- here we add a new embedded_type
        "line_section":{
            "from": {
              // a stop area
            },
            "to": {
              // a stop area
            },
            "line": {
               // the line
            }
            // for the moment the idea is not to dump the associated routes
        },
        "quality":0,
        "id":"id_of_the_line_section",
        "name":"Line section"
      }
    }
  ]
  "id":"002def5c-dc76-11e6-8542-005056a47b86"
}
```

with this we need to choose whether to dump the stops/line objects (in depth 0) or just make a link

I think dumping the sub objects as depth 0 is more coherent with the rest of the API

This would imply that we provide a `/line_sections/<uri>` api to be coherent, but I think we can skip the whole 
ptref stuf (`/stop_areas/<uri>/line_sections`) as the `line_section` is not really a referential object

## custom line section

Another option would be to add a distinct field (apart from the `impacted_objects`:

```javascript
{
  "status":"active",
  // lots of additional fields
  "impacted_line_sections":[
    {
        "line_section":{
            "from": {
              // a stop area
            },
            "to": {
              // a stop area
            },
            "line": {
               // the line
            }
            // for the moment the idea is not to dump the associated routes
        },
        "quality":0,
        "id":"id_of_the_line_section",
        "name":"Line section"
      }
     }
  ]
  "id":"002def5c-dc76-11e6-8542-005056a47b86"
}
```
This would act the fact that the line section are different than the other objects (thus we can skip the `/line_sections` api), 
but I don't like adding a custom `impacted_line_sections` as it is not coherent with the other impacted objects 
and more important it does not compose well if we want to impact more than a line section.

## hybrid custom line section

There could be a hybrid approach, impacting the line as the main object and adding an `impacted_line_sections` 
(a bit like how delays are handled with `impacted_objects` being a `trip` and `impacted_stops` being the stop_times of this trip)


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
     }
  }
  ]
  "impacted_line_sections":[
  {
      "line_section":{
          "from": {
              // a stop area
          },
          "to": {
              // a stop area
          },
          "line": {
              // the line
          }
          // routes ?
      },
  },
  ]

```

With this we can skip the `/line_sections` api as it is not treated as a ptobject. I find it also a bit more coherent.

The downside is that the integration is harder, the user needs to check wheter or not a field `impacted_line_sections` is present to treat the impacted line differently.

As several objects can be impacted it arise several other problems:
 * is a bit tricky to link the line to its line section
 * if a impact is link to the "line_1" AND to a line section of this line (I don't see the point of doing this, but it's technically possible), it's difficult to represent this


## hybrid custom line section #2

It's the same as above, but @pbougue proposed to add a `non_pt_objects` aside the `impacted_objects`

I don't really like this as it will either break the delay api (`impacted_stops`), or have another field aside.

## route sections

The real impacted objects on the line_sections are routes's sections

Thus we could represent the line section as a routes section

```javascript
{
  "status":"active",
  // lots of additional fields
  "impacted_objects":[
    {
      "pt_object":{
        "embedded_type":"route_section", // <--- here we add a new embedded_type
        "line_section":{
            "from": {
              // a stop area
            },
            "to": {
              // a stop area
            },
            "routes": [{
               // one route
            }, {
               // another route
            }]
            // to ease integration we could also add the impacted lines here
        },
        "quality":0,
        "id":"id_of_the_route_section",
        "name":"Route section"
      }
    }
  ]
  "id":"002def5c-dc76-11e6-8542-005056a47b86"
}
```

I find it less constrained by how the disruption is created in the TR ui but one major problem with this is that the client asking us to work 
on line section can a lot's of impacted routes (+100), thus it can flood the response and cause major performance problems

## hybrid route sections

The same hybrid approach can be used for route sections

```javascript
{
  "status":"active",
  // lots of additional fields
  "impacted_objects":[
  {
     "pt_object": {
        "embedded_type": "route",
        "route": {
          // a route
        }
        "id":"id_of_the_route",
     }
  },
  {
     "pt_object": {
        "embedded_type": "route",
        "route": {
          // another route
        }
        "id":"id_of_the_route_2",
     }
  }
  ]
  "impacted_route_sections":[
  {
      "route_section":{
          "from": {
              // a stop area
          },
          "to": {
              // a stop area
          },
          "route": {
              // the line
          }
      },
  },
  ]

```

As there can be many many routes I don't think this approach can work


# when to display the impact on a line section

## pt ref

I think an example will be easier to understand. We impact the stops [C, D, E] of the 'route_1_of_the_line_1'

```
A           B           C           D           E           F
o --------- o --------- o --------- o --------- o --------- o   <- route_1_of_the_line_1

                        XXXXXXXXXXXXXXXXXXXXXXXXX               <- impact on the route_1_of_the_line_1 on [C, E]
```

### common cases

I think in a v1 (but I don't think we'll do a v2 :wink: ) we can keep it quite simple (but not that coherent :confused: ).

On ptref we can display the line section disruption on a ptref call if the last object of the ptref call is directly linked to an **active** disruption

examples:

this is the navitia call and YES means we display the disruption, NO means we do not display it

`/routes/route_1_of_the_line_1` --> YES

`/stop_areas/A`  --> NO

`/stop_areas/B`  --> NO

`/stop_areas/C`  --> YES

`/stop_areas/D`  --> YES

`/stop_areas/E`  --> YES

`/stop_areas/F`  --> NO

`/routes/route_2_of_the_line_1`  --> NO

`/lines/line_1`  -->  YES

`/lines/line_2`  -->  NO

If we decide to display the line section disruptions based on the last object of the query it will lead to some difficult to understand behaviour:

`/stop_areas/A/routes/route_1_of_the_line_1`  --> YES

`/stop_areas/C/routes/route_2_of_the_line_1`  --> NO

`/networks/network_of_the_route_1 `  --> NO

`/stop_points/C_1`  --> YES

`/stop_points/C_2_of_route_2_of_line_1`  --> NO

`/stop_points/C_3_of_line_2`  --> NO

`/vehicle_journeys/vj1_of_route_1_of_line_1`  --> NO


### ptref with /disruptions

This is for the calls api.navitia.io/v1/coverage/bob/<some pt ref filters>/disruptions

The /disruption api is meant to be a direct representation of the object model below so I think it's ok the keep the same mechanism as before 
where only the objects with disruptions technically linked to them have disruptions.

So this will depend on the chosen line section model

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


## schedules

I don't think we need to do some stuff here, the same rules as the common cases can be used.

## journeys

If a journey use a VJ of an impacted route on at least one stop point impacted by the line section we display it.


# modelisation

TODO, we need to first agree on the other questions before doing this
