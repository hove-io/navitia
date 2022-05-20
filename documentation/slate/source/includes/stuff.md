Misc mechanisms (and few boring stuff)
======================================

Multiple journeys
-----------------

Navitia can compute several kind of journeys with a journey query.

The
[RAPTOR](http://research.microsoft.com/apps/pubs/default.aspx?id=156567)
algorithm used in Navitia is a multi-objective algorithm. Thus it might
return multiple journeys if it cannot know that one is better than the
other. For example it cannot decide that a one hour journey with no
connection is better than a 45 minutes journey with one connection
(it is called the [pareto front](http://en.wikipedia.org/wiki/Pareto_efficiency)).
The 3 objectives Navitia uses are roughly the arrival datetime, the number of transfers
and the duration of "walking" (transfers and fallback).

If the user asks for more journeys than the number of journeys given by
RAPTOR (with the parameter `min_nb_journeys` or `count`), Navitia will
ask RAPTOR again, but for the following journeys (or the previous ones
if the user asked with `datetime_represents=arrival`).

Those journeys have the `next` (or `previous`) value in their tags.

Journey qualification process
-----------------------------

Since Navitia can return several journeys, it tags them to help the user
choose the best one that matches their needs.
Here are some tagging rules:
- there is only one "best" itinerary
- itineraries with other types are displayed **only if they are relevant**
- and for a specific type, you may find many relevant itinerary

For example, for a specific request, you can find the "best" itinerary, 2 "less_fallback_walk" ones
(with less walking, but taking more time) and no "comfort" (the "best" one is the same as the "comfort" one for example).

The different journey types are:

|Type|Description|
|----|-----------|
|best|The best journey if you have to display only one.|
|rapid|A good trade off between duration, changes and constraint respect|
|comfort|A journey with less changes and walking|
|car|A journey with car to get to the public transport|
|less_fallback_walk|A journey with less walking|
|less_fallback_bike|A journey with less biking|
|less_fallback_bss|A journey with less bss|
|fastest|A journey with minimum duration|
|bike_in_pt|A journey with bike both at the beginning and the end, and where bike is allowed in public transport used|
|non_pt_walk|A journey without public transport, only walking|
|non_pt_bike|A journey without public transport, only biking|
|non_pt_bss|A journey without public transport, only bike sharing|

<h2 id=Service-translation>Service Translation</h2>

When specifying a service in the data through `calendar.txt` and `calendar_dates.txt`, you might get surprised to see a different result from Navitia's response.

```shell
# Navitia's interpreted service response
{
    "calendars": [
       {
            "active_periods": [
                {
                    "begin": "20200101",
                    "end": "20200130"
                }
            ],
            "week_pattern": {
                "monday": false,
                "tuesday": false,
                "wednesday": false,
                "thursday": false,
                "friday": false,
                "saturday": true,
                "sunday": true
            },
            "exceptions": [
                {
                    "type": "remove",
                    "datetime": "20200127"
                }
            ]
        }
    ]
```

For instance, if you have a trip that:

-   occurs every Saturday of January 2020 (in your `calendar.txt`)
-   has 3 exceptions that `add` the service the first 3 Sundays (`calendar_dates.txt`)

You might expect to have that same exact representation for your `/vehicle_journeys`, but instead you find something different.

You expected to have 3 `add` exceptions but you only have 1 `remove`. Also, the week pattern is set to `true` on Sunday even though your `calendar.txt` said Saturday only.

This is due to a re-interpretation of the service pattern, with one specific goal:

<aside class="notice">
 Navitia analyzes the input service and computes an equivalent pair of (week_pattern, exceptions) with the **smallest** number of exceptions
</aside>

This means that Navitia will re-organise the input data, to produce a smaller and more comprehensive response that respects the semantics of the input data.

For more information about the algorithm (in French): [https://github.com/TeXitoi/pinot2015presenter/blob/master/pres/pinot2015presenter-pres.pdf](https://github.com/TeXitoi/pinot2015presenter/blob/master/pres/pinot2015presenter-pres.pdf)

<h2 id="ridesharing-stuff">Ridesharing</h2>

``` shell
simplified output

{
    "journeys": [
        {
            "requested_date_time": "20180101T070000",
            "sections": [
                {
                    "type": "street_network",
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

When requesting a journey, it is possible to request for a ridesharing fallback,
using `first_section_mode` or `last_section_mode`.
This may also be used to obtain a direct ridesharing journey (using `max_ridesharing_duration_to_pt=0`).

This returns a journey only when one or multiple ridesharing ads are found, matching the request.

<aside class="warning">
    This feature requires a specific configuration and an agreement from a ridesharing service provider.
    Therefore this service is available only for a few clients.
</aside>

The journey from Navitia will then contain a section using the ridesharing mode.
Inside this section an attribute ridesharing_journeys contains one or multiple journeys
depicting specifically the ridesharing ads that could match the above section
and that could be proposed to the user.

<h2 id="taxi-stuff">Taxi</h2>

<aside class="warning">
    This feature is not available on all coverages, as it is dependent on other parameters of the coverage (how journeys are computed).
</aside>

With this mode, your journey may contain taxi sections(fallback or direct path). The journey you will obtain is basically the same as a journey by car. The only difference is that with taxi as fallback mode, a buffer time (section "waiting", defaulted to 5 min) will appear into the journey. The buffer time won't appear if the journey is a direct path. Depending on the calculator, the journey may pick up ways that are reserved for taxis on not.

<h2 id="odt">On demand transportation</h2>

Some transit agencies force travelers to call them to arrange a pickup at a particular place or stop point.

Besides, some stop times can be "estimated" *in data by design*:

- A standard GTFS contains only regular time: that means transport agencies should arrive on time :)
- But navitia can be fed with more specific data, where "estimated time" means that there will be
no guarantee on time respect by the agency. It often occurs in suburban or rural zone.

After all, the stop points can be standard (such as bus stop or railway station)
or "zonal" (where agency can pick you up you anywhere, like a cab).

That's some kind of "responsive locomotion" (ɔ).

So public transport lines can mix different methods to pickup travelers:

-   regular
    -   line does not contain any estimated stop times, nor zonal stop point location.
    -   No need to call too.
-   odt_with_stop_time
    -   line does not contain any estimated stop times, nor zonal stop point location.
    -   But you will have to call to take it.
-   odt_with_stop_point
    -   line can contain some estimated stop times, but no zonal stop point location.
    -   And you will have to call to take it.
-   odt_with_zone
    -   line can contain some estimated stop times, and zonal stop point location.
    -   And you will have to call to take it
    -   well, not really a public transport line, more a cab...

<h2 id="disruptions">Disruptions</h2>

In this paragraph, we will explain how the disruptions are displayed in the different APIs.

### publication period of a disruption
The publication period of a disruption is the period on which we want to display the disruption in navitia.

The creator of the disruption might not want the traveller to know of a disruption before a certain date (because it's too uncertain, secret, ...)
The publication period is the way to control this.

### application periods of a disruption
The application periods are the list of periods on which the disruption is active.
For example it's the actual period when railworks are done and train circulation is cut.

### Request date

The request date represents datetime for Journeys API, from_datetime for  Schedules API or now for the others APIs.
The default value is now.

### Summary

To sum up we display an impact if 'now' is in the publication period.

The status of the impact depends only of 'now' and is:

* 'active' if 'now' is inside an application period
* 'future' if 'now' is not inside an application period and there is an application period after 'now'
* 'past' otherwise

<table>
  <thead>
    <tr>
      <th align="center" colspan="2"></th>
      <th align="center"> </th>
      <th align="center" colspan="3">Show impacts </th>
      <th align="center"> </th>
    </tr>
    <tr>
      <th align="center">publication period </th>
      <th>Application period </th>
      <th align="center">Request date</th>
      <th align="center">disruptions API</th>
      <th align="center">traffic_reports & PtRef API</th>
      <th align="center">Journeys & Schedules API</th>
      <th align="center">Status</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center">date1 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center">- </td>
      <td align="center">- </td>
      <td align="center">future </td>
    </tr>
    <tr>
      <td align="center">T </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td align="center">| </td>
      <td align="center"> </td>
      <td align="center">date2 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"> </td>
      <td align="center">future </td>
    </tr>
    <tr>
      <td align="center">| </td>
      <td align="center">T </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td align="center">| </td>
      <td align="center">| </td>
      <td align="center">date3 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center">active </td>
    </tr>
    <tr>
      <td align="center">| </td>
      <td align="center">⊥ </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td align="center">| </td>
      <td> </td>
      <td align="center">date4 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"> </td>
      <td align="center">passed </td>
    </tr>
    <tr>
      <td align="center">⊥ </td>
      <td> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td align="center"> </td>
      <td> </td>
      <td align="center">date5 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center">- </td>
      <td align="center">- </td>
      <td align="center">passed </td>
    </tr>
  </tbody>
</table>
