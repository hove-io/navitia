<a name="realtime"></a> Real time integration in Navita
===============================================

Several endpoints can integrate real time information in their responses. In the response received, a disruption will be present and additional information will be provided if the parameter `data_freshness` is set to `realtime`. If the parameter `data_freshness` is set to `base_schedule`, the disruption is also present in the response for the user information, but it won't be taken into account in the results of the query.

<aside class="warning">
Real time isn't available on every coverage for Navitia. For real time to be available for a client, it needs to provide real time info about its network to Navitia.
</aside>

The effect of a disruption can be one of the following:
<ul>
	<li>[SIGNIFICANT_DELAYS](#SIGNIFICANT_DELAYS)</li>
	<li>[REDUCED_SERVICE](#REDUCED_SERVICE)</li>
</ul>

For each one of these effects, here's how the Navitia responses will be affected over the different endpoints. A link to the API


## <a name="SIGNIFICANT_DELAYS"></a>Trip delayed

![image](delay.png)

``` shell
# Extract of an impacted stop of /disruptions
{
    "amended_arrival_time": "194700",
    "amended_departure_time": "194900",
    "arrival_status": "delayed",
    "base_arrival_time": "193200",
    "base_departure_time": "193400",
    "cause": "Panne d'un aiguillage",
    "departure_status": "delayed",
    "is_detour": false,
    "stop_point": ⊕{7 items},
    "stop_time_effect": "delayed",
}
```
The effect of the disruption is `SIGNIFICANT_DELAYS`. It means that the train will arrive late at one or more stations in its journey.

In the disruption, the delay can be found in the list of "impacted_stops" with the departure/arrival status set to "delayed". See the [disruption](#disruption) objects section for its full content and description.

<div></div>
### Journeys

``` shell
# Request example for /journeys
http://api.navitia.io/v1/coverage/<coverage>/journeys?from=<origin>&to=<destination>&data_freshness=realtime
```

``` shell
# Extract of the public transport section of the response /journeys

    "sections": ⊖[
       ⊖{
            "additional_informations": ⊕[1 item],
            "arrival_date_time": "20190529T205000",
            "base_arrival_date_time": "20190529T204500",
            "base_departure_date_time": "20190529T160000",
            "co2_emission": ⊕{2 items},
            "data_freshness": "realtime",
            "departure_date_time": "20190529T160000",
            "display_informations": ⊕{13 items},
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


The status of the journey is `SIGNIFICANT_DELAYS`. In a public transport section of the response, "base_arrival_date_time"/"base_departure_date_time" represent the scheduled arrival/departure time without taking into account the delay whereas "arrival_date_time"/"departure_date_time" are the actual arrival/departure time, after the delay is applied. The delay can also be observed for every stop_point of the journey with the same parameters in "stop_date_times".
If the parameter "data_freshness" is set to "base_schedule",  "base_arrival_date_time"/"base_departure_date_time" = "arrival_date_time"/"departure_date_time".

A list of the disruptions impacting the journey is also present at the root level of the response.

<aside class="notice">
For a disruption to be present in the response, the request has to be made during its application period.
</aside>

<div></div>
### Departures

``` shell
# Request example for /departures
http://api.navitia.io/v1/coverage/<coverage>/physical_modes/<physical_mode>/stop_points/<stop_point>/departures?from_datetime=<from_date>&data_freshness=realtime
```

``` shell
# Extract of an impacted departure object of the response /departures

{
    "display_informations": ⊕{13 items},
    "links": ⊕[6 items],
    "route": ⊕{9 items},
    "stop_date_time": ⊖{
        "additional_informations": [],
        "arrival_date_time": "20190605T194700",
        "stop_date_time": "realtime"
        "departure_date_time": "20190605T194900",
        "links": [],
    }
    "stop_point": ⊕{11 items},
}
```

In the "stop_date_time" section of the response, the parameter "stop_date_time" is "realtime" and the fields "arrival_date_time"/"departure_date_time" take the delay into account.

A list of the disruptions impacting the departures is also present at the root level of the response.

<div></div>
### Stop Schedules

``` shell
# Request example for /stop_schedules
http://api.navitia.io/v1/coverage/<coverage>/physical_modes/<physical_mode>/lines/<line>/stop_points/<stop_point>/stop_schedules?from_datetime=<from_date>&data_freshness=realtime
```

``` shell
# Extract of an impacted stop_schedules object of the response /stop_schedules

{
    "additional_informations": null,
    "date_times": ⊖[
       ⊕{5 items},
       ⊖{
            "additional_informations": [],
            "data_freshness": "realtime",
            "date_time": "20190605T194900",
            "links": ⊕[2 items]
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

In the list of "date_times" available in the response, the parameter "data_freshness" is "realtime" and the fileld "departure_date_time" take the delay into account.

A list of the disruptions impacting the stop schedules is also present at the root level of the response.

## <a name="REDUCED_SERVICE"></a>Reduced service

![image](reduced_service.png)

``` shell
# Extract of an impacted stop of /disruptions
{
    "arrival_status": "deleted",
    "cause": "",
    "departure_status": "deleted",
    "is_detour": false,
    "stop_point": ⊕{7 items},
    "stop_time_effect": "deleted",
}
```
The effect of the disruption is `REDUCED_SERVICE`. It means that the train won't be serving one or more stations in its journey.

In the disruption, the deleted stations can be found in the list of "impacted_stops" with the departure/arrival status set to "deleted". See the [disruption](#disruption) objects section for its full content and description.

### Journeys

If the station deleted is the destination of the journey, Navitia will compute an other itinerary to the requested station. If not, the disruption will be present at the root level of the response for information, but it won't be affecting the journey.

### Departures & Stop Schedules

The departure time of the train with the reduced service simply won't be displayed in the list of departures/stop_schedules if "data_freshness" is set to "realtime".
