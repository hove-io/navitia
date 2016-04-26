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
-    there is only one "Best" itinerary. It is the ASAP one, on best time
-    the other kinds of itineraries are displayed only if the are relevant
-    and for a specific tag, you may find many relevant itinerary

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
|non_pt_walk|A journey without public transport, only walking|
|non_pt_bike|A journey without public transport, only biking|
|non_pt_bss|A journey without public transport, only bike sharing|

On demand transportation
------------------------

Some transit agencies force travelers to call them to arrange a pickup at a particular place or stop point.

Besides, some stop times can be "estimated" *in data by design* :

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
