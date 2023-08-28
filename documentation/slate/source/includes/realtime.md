<h1 id="realtime">Real time integration in Navita</h1>

Several endpoints can integrate real time information in their responses. In the response received, disruptions can be present and additional information can be provided.
The parameter `data_freshness` can be set to
<ul>
    <li>`base_schedule`: disruptions may be present in the response for the user information, but it won't be taken into account in the results of the query.</li>
    <li>`realtime`: disruptions are taken into account to compute alternative journeys for exemple. In this configuration, every stop times, connexions, delays, or detour can be taken into account.</li>
</ul>

<aside class="notice">
`data_freshness=adapted_schedule` is deprecated and must not be providen. It can only be used for debbuging.
</aside>

<aside class="warning">
Real time isn't available on every coverage for Navitia. For real time to be available for a client, it needs to provide real time info about its network to Navitia.
</aside>

The effect of a disruption can be among the following:
<ul>
    <li>[SIGNIFICANT_DELAYS](#SIGNIFICANT_DELAYS)</li>
    <li>[REDUCED_SERVICE](#REDUCED_SERVICE)</li>
    <li>[NO_SERVICE](#NO_SERVICE)</li>
    <li>[MODIFIED_SERVICE](#MODIFIED_SERVICE)</li>
    <li>[ADDITIONAL_SERVICE](#ADDITIONAL_SERVICE)</li>
    <li>[UNKNOWN_EFFECT](#UNKNOWN_EFFECT)</li>
    <li>[DETOUR](#MODIFIED_SERVICE)</li>
    <li>[OTHER_EFFECT](#OTHER_EFFECT)</li>
</ul>

This list follows the GTFS RT values documented at <https://gtfs.org/reference/realtime/v2/#enum-effect>.

For each one of these effects, here's how the Navitia responses will be affected over the different endpoints.

<aside class="notice">
A disruption is present in the response of the endpoints described if the request is made during its application period.
</aside>

<h2 id="PT_object_collections_data_freshness">Public transport object collections</h2>

Several public transport objects have separate collections for `base_schedule` and `realtime`.<br>So the data_freshness parameter may affect the number of objects returned depending on the request.

For example when looking for a specific circulation with the collection vehicle_journey using the request:<br>`http://api.navitia.io/v1/coverage/<toto>/vehicle_journeys?since=20191008T100000&until=20191008T200000&data_freshness=base_schedule`.

A vehicle_journey circulating between since and until that is **fully deleted** (NO_SERVICE) by a disruption will
of course be **visible** if `data_freshness=base_schedule`.<br>But it **will not appear** with the parameter `data_freshness=realtime` as it does not exist in that collection.

On the other hand, a vehicle_journey that is **created** by a realtime feed will only be **visible** if
`data_freshness=realtime` on that same request.<br>And it will **not appear** if `data_freshness=base_schedule`.

<h2 id="OTHER_EFFECT">Other effect</h2>

There is no known effect related to this disruption. You only have to show the message to your traveler...

<h2 id="SIGNIFICANT_DELAYS">Trip delayed</h2>

![image](delay.png)

``` shell
# Extract of an impacted stop from /disruptions
{
    "amended_arrival_time": "194700",
    "amended_departure_time": "194900",
    "arrival_status": "delayed",
    "base_arrival_time": "193200",
    "base_departure_time": "193400",
    "cause": "Panne d'un aiguillage",
    "departure_status": "delayed",
    "stop_point": ⊕{7 items},
    "stop_time_effect": "delayed",
}
```

The effect of the disruption is `SIGNIFICANT_DELAYS`. It means that the train will arrive late at one or more stations in its journey.

In the disruption, the delay can be found in the list of "impacted_stops" with the departure/arrival status set to "delayed".

* "base_arrival_time"/"base_departure_time" represent the scheduled arrival/departure time without taking into account the delay
* whereas "amended_arrival_time"/"amended_departure_time" are the actual arrival/departure time, after the delay is applied

See the [disruption](#disruption) objects section for its full content and description.

<div></div>

### Journeys

``` shell
# Request example for /journeys
http://api.navitia.io/v1/coverage/<coverage>/journeys?from=<origin>&to=<destination>&data_freshness=realtime
```

``` shell
# Extract of the public transport section from the response /journeys

    "sections": ⊖[
       ⊖{
            "additional_informations": ⊕[1 item],
            "arrival_date_time": "20190529T205000",
            "base_arrival_date_time": "20190529T204500",
            "base_departure_date_time": "20190529T160000",
            "co2_emission": ⊕{2 items},
            "data_freshness": "realtime",
            "departure_date_time": "20190529T160000",
            "display_informations": ⊖{
                "code": "",
                "color": "000000",
                "commercial_mode": "TGV INOUI",
                "description": ""
                "direction": "Nice Ville (Nice)",
                "equipments": [],
                "headsign": "847520",
                "label": "Paris - Nice",
                "links": ⊖[
                   ⊖{
                        "id": "b59bdab8-3560-4cfe-8009-b0461f74c417",
                        "internal": true,
                        "rel": "disruptions",
                        "templated": false
                        "type": "disruption",
                    }
                ],
                "name": "Paris - Nice",
                "network": "SNCF",
                "physical_mode": "Train grande vitesse",
                "text_color": "",
            },
            "duration": 21180,
            "from": ⊕{5 items},
            "geojson": ⊕{3 items},
            "id": "section_0_0",
            "links": ⊕[6 items],
            "stop_date_times": ⊖[
               ⊕{7 items},
               ⊖{
                    "additional_informations": [],
                    "arrival_date_time": "20190605T194700",
                    "base_arrival_date_time": "20190605T193200",
                    "base_departure_date_time": "20190605T193400"
                    "departure_date_time": "20190605T194900",
                    "links": [],
                    "stop_point": ⊕{7 items},
                },
               ⊕{7 items},
               ⊕{7 items},
            ]
            "to": ⊕{5 items},
            "type": "public_transport",
        }
    ]
```

The status of the journey is `SIGNIFICANT_DELAYS`.

In a public transport section of the response:

* "base_arrival_date_time"/"base_departure_date_time" represent the scheduled arrival/departure time without taking into account the delay
* whereas "arrival_date_time"/"departure_date_time" are the actual arrival/departure time, after the delay is applied

The delay can also be observed for every stop point of the journey with the same parameters in "stop_date_times".<br>If the parameter "data_freshness" is set to "base_schedule", then "base_arrival_date_time"/"base_departure_date_time" = "arrival_date_time"/"departure_date_time".

A list of the disruptions impacting the journey is also present at the root level of the response.<br>A link to the concerned disruption can be found in the section "display_informations".

<div></div>

### Departures

``` shell
# Request example for /departures (data_freshness=realtime by default)
http://api.navitia.io/v1/coverage/<coverage>/physical_modes/<physical_mode>/stop_areas/<stop_area>/departures?from_datetime=<from_date>&data_freshness=realtime
```

``` shell
# Extract of an impacted departure object from the response /departures

{
    "display_informations": ⊖{
        "code": "",
        "color": "000000",
        "commercial_mode": "TGV INOUI",
        "description": ""
        "direction": "Nice Ville (Nice)",
        "equipments": [],
        "headsign": "847520",
        "label": "Paris - Nice",
        "links": ⊖[
           ⊖{
                "id": "b59bdab8-3560-4cfe-8009-b0461f74c417",
                "internal": true,
                "rel": "disruptions",
                "templated": false
                "type": "disruption",
            }
        ],
        "name": "Paris - Nice",
        "network": "SNCF",
        "physical_mode": "Train grande vitesse",
        "text_color": "",
    },
    "links": ⊕[6 items],
    "route": ⊕{9 items},
    "stop_date_time": ⊖{
        "additional_informations": [],
        "arrival_date_time": "20190605T194700",
        "base_arrival_date_time": "20190605T193200",
        "base_departure_date_time": "20190605T193400",
        "stop_date_time": "realtime"
        "departure_date_time": "20190605T194900",
        "links": [],
    }
    "stop_point": ⊕{11 items},
}
```

In the "stop_date_time" section of the response, the parameter "stop_date_time" is "realtime" and the fields "arrival_date_time"/"departure_date_time" take the delay into account, whereas "base_arrival_date_time"/"base_departure_date_time" show the base-schedule departure/arrival datetime.

A list of the disruptions impacting the departures is also present at the root level of the response.<br>A link to the concerned disruption can be found in the section "display_informations".

<div></div>

### Stop Schedules

``` shell
# Request example for /stop_schedules (data_freshness=realtime by default)
http://api.navitia.io/v1/coverage/<coverage>/physical_modes/<physical_mode>/lines/<line>/stop_areas/<stop_area>/stop_schedules?from_datetime=<from_date>&data_freshness=realtime
```

``` shell
# Extract of an impacted stop_schedules object from the response /stop_schedules

{
    "additional_informations": null,
    "date_times": ⊖[
       ⊕{5 items},
       ⊖{
            "additional_informations": [],
            "base_date_time": "20190605T193400",
            "data_freshness": "realtime",
            "date_time": "20190605T194900",
            "links": ⊖[
               ⊕{4 items},
               ⊖{
                    "id": "b59bdab8-3560-4cfe-8009-b0461f74c417",
                    "internal": true,
                    "rel": "disruptions",
                    "templated": false
                    "type": "disruption",
                }
            ]
        },
       ⊕{5 items},
    ],
    "display_informations": ⊕{9 items},
    "first_datetime": ⊕{5 items}
    "last_datetime": ⊕{5 items},
    "links": ⊕[4 items],
    "route": ⊕{8 items},
    "stop_point": ⊕{11 items},
}
```

In the list of "date_times" available in the response, the parameter "data_freshness" is "realtime" and the field "date_time" takes the delay into account, whereas "base_date_time" shows the base-schedule departure datetime.

A list of the disruptions impacting the stop schedules is also present at the root level of the response.<br>A link to the concerned disruption can be found in the in the "date_times" object itself.

<h2 id="REDUCED_SERVICE">Reduced service</h2>

![image](reduced_service.png)

``` shell
# Extract of an impacted stop from /disruptions
{
    "arrival_status": "deleted",
    "cause": "",
    "departure_status": "deleted",
    "stop_point": ⊕{7 items},
    "stop_time_effect": "deleted",
}
```
The effect of the disruption is `REDUCED_SERVICE`. It means that the train won't be serving one or more stations in its journey.

In the disruption, the deleted stations can be found in the list of "impacted_stops" with the departure/arrival status set to "deleted".<br>See the [disruption](#disruption) objects section for its full content and description.

<div></div>

### Journeys

If the stop deleted is the origin/destination of a section of the journey, Navitia will compute a different itinerary to the requested station.<br>If not (deleting intermediate stop), the journey won't be affected.<br>Either way, a link to this disruption can be found in the section "display_informations" like for other disruptions.

### Departures & Stop Schedules

At the deleted stop area, the departure time of the train with a reduced service simply won't be displayed in the list of departures/stop_schedules if "data_freshness" is set to "realtime".

If "data_freshness" is "base_schedule", then the depature time is displayed.<br>In that case, a link to this disruption can be found in the section "display_informations" for departures, in the "date_times" object itself for stop_schedules.

<h2 id="NO_SERVICE">No service</h2>

![image](no_service.png)

``` shell
# Extract of an impacted trip from /disruptions
{
    "application_periods": ⊖[
       ⊖{
            "begin": "20191210T183100",
            "end": "20191210T194459"
        }
    ],
    "impacted_objects": ⊖[
       ⊖{
            "pt_object": ⊖{
                "embedded_type": "trip",
                "id": "OCE:BOA017534OCESNF-20191210",
                "name": "OCE:BOA017534OCESNF-20191210",
                "trip": ⊕{2 items}
            }
        }
    ],
    "severity": ⊖{
        "effect": "NO_SERVICE"
        "name": "trip canceled",
    },
}
```

The effect of the disruption is `NO_SERVICE`. It means that the train won't be circulating at all.

In the disruption, the deleted trip can be found in the "impacted_objects" list with the
"application_periods" describing the period(s) of unavailability for the trip.<br>See the [disruption](#disruption) objects section for its full content and description.

<div></div>

### Journeys

If the deleted trip is used by a section of the `base_schedule` journey, Navitia will compute a different
itinerary to the requested station in `realtime`, or a later one (without using that trip).<br>A link to this disruption can be found in the section "display_informations" like for other disruptions, on a `base_schedule` journey.

### Departures & Stop Schedules

At the deleted stop area, the departure time of the cancelled train simply won't be displayed in the list of departures/stop_schedules if "data_freshness" is set to "realtime".

If "data_freshness" is "base_schedule", then the depature time is displayed.<br>In that case, a link to this disruption can be found in the section "display_informations" for departures, in the "date_times" object itself for stop_schedules.

<h2 id="MODIFIED_SERVICE">Modified service</h2>

![image](modified_service.png)

``` shell
# Extract of an impacted stop from /disruptions
{
    "amended_arrival_time": "185500",
    "amended_departure_time": "190000",
    "arrival_status": "added"
    "cause": "Ajout d'une desserte",
    "departure_status": "added",
    "stop_point": ⊕{7 items},
    "stop_time_effect": "added",
},
```
The effect of the disruption is `MODIFIED_SERVICE`. It means that there is one or several stop points added into the trip. This can be at any position in the trip (origin and destination included).

In the disruption, new stop points can be found in the list of "impacted_stops" with the departure/arrival status set to "added". The scheduled arrival/departure at the new stop point can be found in "amended_arrival_time"/"amended_departure_time".<br>See the [disruption](#disruption) objects section for its full content and description.

<div></div>

### Journeys

``` shell
# Request example for /journeys
http://api.navitia.io/v1/coverage/<coverage>/journeys?from=<origin>&to=<destination>&data_freshness=realtime
```

``` shell
# Extract of the public transport section from the response /journeys

    "sections": ⊖[
       ⊖{
            "additional_informations": ⊕[1 item],
            "arrival_date_time": "20190605T204500",
            "base_arrival_date_time": "20190605T204500",
            "co2_emission": ⊕{2 items},
            "data_freshness": "realtime",
            "departure_date_time": "20190605T160000",
            "display_informations": ⊖{
                "code": "",
                "color": "000000",
                "commercial_mode": "TGV INOUI",
                "description": ""
                "direction": "Nice Ville (Nice)",
                "equipments": [],
                "headsign": "847520",
                "label": "Paris - Nice",
                "links": ⊖[
                   ⊖{
                        "id": "b59bdab8-3560-4cfe-8009-b0461f74c418",
                        "internal": true,
                        "rel": "disruptions",
                        "templated": false
                        "type": "disruption",
                    }
                ],
                "name": "Paris - Nice",
                "network": "SNCF",
                "physical_mode": "Train grande vitesse",
                "text_color": "",
            },
            "duration": 28560,
            "from": ⊕{5 items},
            "geojson": ⊕{3 items},
            "id": "section_1_0",
            "links": ⊕[6 items],
            "stop_date_times": ⊖[
                ⊕{7 items},
                ⊖{
                     "additional_informations": [],
                     "arrival_date_time": "20190605T185500",
                     "departure_date_time": "20190605T190000",
                     "links": []
                     "stop_point": ⊕{7 items},
                 },
                ⊕{7 items},
            ]
            "to": ⊕{5 items},
            "type": "public_transport",
        },
    ]
```

The status of the journey is `MODIFIED_SERVICE`. In a public transport section of the response, "arrival_date_time"/"departure_date_time" are the arrival/departure times of an added stop point. New stop points are only used when the "data_freshness" parameter is set to "realtime".

A list of the disruptions impacting the journey is also present at the root level of the response.<br>A link to the concerned disruption can be found in the section "display_informations".

<div></div>

### Departures & Stop Schedules

At the added stop area, the departure time of the train with a modified service is displayed if "data_freshness" is set to "realtime".<br>In that case, a link to this disruption can be found in the section "display_informations" for departures, in the "date_times" object itself for stop_schedules.

The departure time of the train with a modified service simply won't be displayed in the list of departures/stop_schedules if "data_freshness" is set to "base_schedule".

<h2 id="ADDITIONAL_SERVICE">Additional service</h2>

![image](additional.png)

``` shell
# Extract of an impacted stop from /disruptions
{
    "amended_arrival_time": "193200",
    "amended_departure_time": "193400",
    "arrival_status": "added"
    "cause": "",
    "departure_status": "added",
    "is_detour": false,
    "stop_point": ⊕{7 items},
    "stop_time_effect": "added",
}
```

The effect of the disruption is `ADDITIONAL_SERVICE`. It means that a new trip has been scheduled.

In the disruption, every stops served by the new train can be found in the list of "impacted_stops" with the departure/arrival status set to "added". The scheduled arrival/departure at the new stop point can be found in "amended_arrival_time"/"amended_departure_time".<br>See the [disruption](#disruption) objects section for its full content and description.

<div></div>

### Journeys

``` shell
# Request example for /journeys
http://api.navitia.io/v1/coverage/<coverage>/journeys?from=<origin>&to=<destination>&data_freshness=realtime
```

``` shell
# Extract of the public transport section from the response /journeys

"sections": ⊖[
       ⊕{10 items},
       ⊖{
            "additional_informations": ⊕[1 item],
            "arrival_date_time": "20190710T204500",
            "co2_emission": ⊕{2 items},
            "data_freshness": "realtime",
            "departure_date_time": "20190710T160000",
            "display_informations": ⊖{
                "code": "",
                "color": "000000",
                "commercial_mode": "additional service",
                "description": ""
                "direction": "Nice Ville (Nice)",
                "equipments": [],
                "headsign": "20470",
                "label": "Paris - Nice",
                "links": ⊖[
                   ⊖{
                        "id": "1ec4266c-e7f7-4212-a2df-f61c2b56ce91",
                        "internal": true,
                        "rel": "disruptions",
                        "templated": false
                        "type": "disruption",
                    }
                ],
                "name": "Paris - Nice",
                "network": "additional service",
                "physical_mode": "Train grande vitesse",
                "text_color": "FFFFFF",
            },
            "duration": 28560,
            "from": ⊕{5 items},
            "geojson": ⊕{3 items},
            "id": "section_6_0",
            "links": ⊕[6 items],
            "stop_date_times": ⊖[
               ⊖{
                    "additional_informations": [],
                    "arrival_date_time": "20190710T160000",
                    "departure_date_time": "20190710T160000",
                    "links": []
                    "stop_point": ⊕{7 items},
                },
               ⊕{5 items},
               ⊕{5 items},
               ⊕{5 items}
            ]
            "to": ⊕{5 items},
            "type": "public_transport",
        },
       ⊕{10 items}
    ]
```

The status of the journey is `MODIFIED_SERVICE`. This new journey can only be displayed if "data_freshness" is set to "realtime".<br>A list of disruptions impacting the journey is also present at the root level of the response.<br>A link to the concerned disruption can be found in the section "display_informations".

<div></div>

### Departures & Stop Schedules

At one of the added stop area from the additional trip, the departure time of the added train is displayed if "data_freshness" is set to "realtime".<br>In that case, a link to this disruption can be found in the section "display_informations" for departures, in the "date_times" object itself for stop_schedules.

The departure time of the train with an additional service simply won't be displayed in the list of departures/stop_schedules if "data_freshness" is set to "base_schedule".


<h2 id="UNKNOWN_EFFECT">Unknown effect aka "Back to normal"</h2>

``` shell
# Extract of an impacted stop from /disruptions
{
    "amended_arrival_time": "193200",
    "amended_departure_time": "193400",
    "arrival_status": "unchanged"
    "base_arrival_time": "193200",
    "base_departure_time": "193400",
    "cause": "",
    "departure_status": "unchanged",
    "is_detour": false,
    "stop_point": ⊕{7 items},
    "stop_time_effect": "unchanged",
},
```
The effect of the disruption is `UNKNOWN_EFFECT`. It means that the disruption affecting the journey is no longer effective, and the trip is back to its theoritical schedule.<br>In the list of "impacted_stops" in the disruption, the "arrival_status"/"departure_status" is set to "unchanged".<br>See the [disruption](#disruption) objects section for its full content and description.

<div></div>

### Journeys

When requesting a journey that was previously disrupted and is now back to normal, the journey response will be the same with the parameter "data_freshness" set to "realtime" or to "base_schedule".<br>In this case, no disruption is present in the response.

<div></div>

### Departures

``` shell
# Request example for /departures (data_freshness=realtime by default)
http://api.navitia.io/v1/coverage/<coverage>/physical_modes/<physical_mode>/stop_areas/<stop_area>/departures?from_datetime=<from_date>&data_freshness=realtime
```

``` shell
# Extract of an impacted departure object from the response /departures

{
    "display_informations": ⊖{
        "code": "",
        "color": "000000",
        "commercial_mode": "TGV INOUI",
        "description": ""
        "direction": "Nice Ville (Nice)",
        "equipments": [],
        "headsign": "847520",
        "label": "Paris - Nice",
        "links": [],
        "name": "Paris - Nice",
        "network": "SNCF",
        "physical_mode": "Train grande vitesse",
        "text_color": "",
    },
    "links": ⊕[6 items],
    "route": ⊕{9 items},
    "stop_date_time": ⊖{
        "additional_informations": [],
        "arrival_date_time": "20190605T193200",
        "base_arrival_date_time": "20190605T193200",
        "base_departure_date_time": "20190605T193400",
        "stop_date_time": "realtime"
        "departure_date_time": "20190605T193400",
        "links": [],
    }
    "stop_point": ⊕{11 items},
}
```
In the "stop_date_time" section of the response, the field "stop_date_time" is "realtime" and the fields "arrival_date_time"/"departure_date_time" are equal to the fields "base_arrival_date_time"/"base_departure_date_time".

No disruption is present at the root level of the response and so, in the section "display_informations", there's no link to any disruption.

<div></div>

### Stop Schedules

``` shell
# Request example for /stop_schedules (data_freshness=realtime by default)
http://api.navitia.io/v1/coverage/<coverage>/physical_modes/<physical_mode>/lines/<line>/stop_areas/<stop_area>/stop_schedules?from_datetime=<from_date>&data_freshness=realtime
```

``` shell
# Extract of an impacted stop_schedules object from the response /stop_schedules

{
    "additional_informations": null,
    "date_times": ⊖[
       ⊕{5 items},
       ⊖{
            "additional_informations": [],
            "base_date_time": "20190605T193400",
            "data_freshness": "realtime",
            "date_time": "20190605T193400",
            "links": [],
        },
       ⊕{5 items},
    ],
    "display_informations": ⊕{9 items},
    "first_datetime": ⊕{5 items}
    "last_datetime": ⊕{5 items},
    "links": ⊕[4 items],
    "route": ⊕{8 items},
    "stop_point": ⊕{11 items},
}
```

In the list of "date_times" available in the response, the field "data_freshness" is "realtime" and the field "date_time" is equal to the field "base_date_time".

No disruption is present at the root level of the response and so, in the sections "date_times" and "display_informations", there's no link to any disruption.

<div></div>

### Terminus Schedules

``` shell
# Request example for /terminus_schedules (data_freshness=base_schedule by default)
http://api.navitia.io/v1/coverage/<coverage>/physical_modes/<physical_mode>/lines/<line>/stop_areas/<stop_area>/terminus_schedules?from_datetime=<from_date>&data_freshness=realtime
```

``` shell
# Extract of an impacted terminus_schedules object from the response /terminus_schedules
Same elements as in stop_scedule object.
```

In the list of "date_times" available in the response, the field "data_freshness" is "realtime" and the field "date_time" is equal to the field "base_date_time".

No disruption is present at the root level of the response and so, in the sections "date_times" and "display_informations", there's no link to any disruption.
