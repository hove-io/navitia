# Ridesharing


## 1-1 call over a simple crowfly

The simplest way of dealing with ridesharing is to make a crowfly,
then call an external system to "cover" that crowfly section.


### Parameters

A new value possible `ridesharing` will be added to the parameters
`first_section_mode` and `last_section_mode` (not activated by default).

If one only wants direct ridesharing, `max_ridesharing_duration_to_pt` will be added
and can be passed to 0 to deactivate fallback.

In Tyr db, we should also add `max_ridesharing_duration_to_pt` to `instance` and `instance_traveler_types`.


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
                                    "type": "ridesharing"
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
* respects constraints provided in parameters _(journey starting after 7:00)_

Inside that ridesharing-crowfly section we find the `ridesharing_journeys`.  
They are showing multiple possible journeys using ridesharing and covering that crowfly section.  
* Each journey will have a starting and ending crowfly to manage succession of sections in space (even with top-level sections).  
* Time-consistency is respected in that sub-journey _(A @6:30 -> A1 @6:30 -> A2 @9:30 -> B @9:30)_,
* However time-consistency is not respected if an integrator chooses to substitute those sub-journey's sections to the top ridesharing-crowfly section
_(example starts before provided parameter, and ends after the start of public-transport section that follows)_.  
No "waiting" section is provided as it could be Back to the Future _(cf. example)_.


### Fare and CO2 emissions

In that case, the fare and CO2 emissions of the journey containing a ridesharing-crowfly are not impacted
by the ridesharing ads that are presented (we don't know which one would actually be selected by the passenger).


### Configuration and access

The ridesharing will be activated by coverage (and on everyone who has access to that coverage).


### Miscellaneous comments

`network` field of `ridesharing_informations` will probably be used as an id when integrating the response,
to output the proper name of the ridesharing provider, as well as any logo and colors that would be associated to it to customise output.


### Undecided fields

In any case, a decision as to consider RDEX format to avoid providing too much informations not handled by most of ridesharing services.

A `ridesharing_speed` param may appear to manage the speed of ridesharing crowfly.

The `display_informations` field is not filled so far, we could however add informations to make it usable "blindly" as any `display_informations`.

A `passengers` object-list could appear to describe other passengers on the ridesharing-ad. How about personal informations in Navitia?

A `vehicle_informations` object could appear (in driver or in ridesharing_informations) to describe and offer a picture of the driver's car.

Only `"type": "ridesharing"` is provided in the ridesharing-journey's section, not `"mode": "ridesharing"`. We could probably add it if necessary.

We could add a link to the journey after ridesharing if the datetimes doesn't match.
