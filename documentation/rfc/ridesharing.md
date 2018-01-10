# Ridesharing


## 1-1 call over a simple crowfly

The simplest way of dealing with ridesharing is to make a crowfly,
then call an external system to "cover" that crowfly section.


### Output description

The output of a `/journeys` call would follow [this json example](./journeys_ridesharing.json).

Simplified output:
```json
{
    "journeys": [
        {
            "requested_date_time": "20180101T070000",
            "sections": [
                {
                    "type": "crow_fly",
                    "mode": "ridesharing",
                    "from": "A",
                    "to": "B",
                    "departure_date_time": "20180101T070000",
                    "arrival_date_time": "20180101T090000",
                    "ridesharing_journeys": [
                        {
                            "sections":[
                                {
                                    "from": "A",
                                    "to": "A1",
                                    "departure_date_time": "20180101T063000",
                                    "arrival_date_time": "20180101T063000",
                                    "type": "crow_fly",
                                    "mode": "walking"
                                },
                                {
                                    "from": "A1",
                                    "to": "A2",
                                    "departure_date_time": "20180101T063000",
                                    "arrival_date_time": "20180101T093000",
                                    "type": "ridesharing",
                                    "mode": "ridesharing"
                                },
                                {
                                    "from": "A2",
                                    "to": "B",
                                    "departure_date_time": "20180101T093000",
                                    "arrival_date_time": "20180101T093000",
                                    "type": "crow_fly",
                                    "mode": "walking"
                                }
                            ]
                        },
                        {
                            ...
                        }
                    ]
                },
                {
                    "from": "B",
                    "to": "C",
                    "departure_date_time": "20180101T090000",
                    "arrival_date_time": "20180101T100000",
                    "type": "public_transport"
                }
            ]
        },
        {
            ...
        }
    ]
}
```

The main principle is to show a `crowfly` section with `ridesharing` as mode in the journey.  
This allows Navitia to have a succession of sections that:  
* is consistent in both time and space _(A @7:00 -> B @9:00 -> C @10:00)_
* respects constraints provided in parameters _(journey starting after 7:00)_.

Inside that ridesharing-crowfly section we find the `ridesharing_journeys`.  
They are showing multiple possible journeys using ridesharing and covering that crowfly section.  
* Each journey will have a starting and ending crowfly to manage succession of sections in space (even with top-level sections).  
* Time-consistency is respected in that sub-journey _(A @6:30 -> A1 @6:30 -> A2 @9:30 -> B @9:30)_,
* However time-consistency is not respected if an integrator chooses to substitute those sub-journey's sections to the top ridesharing-crowfly section
_(example starts before provided parameter, and ends after the start of public-transport section that follows)_.


### Fare and CO2 emissions

In that case, the fare and CO2 emissions of the journey containing a ridesharing-crowfly are not impacted
by the ridesharing ads that are presented (we don't know which one would actually be selected by the passenger).
