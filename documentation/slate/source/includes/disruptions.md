Disruptions
===========


In this paragraph, we will know how the disruptions are displayed in the different APIs

# Context

## periods

### production period 
The production period is global to a navitia coverage and is the validity period of the coverage's data.
This production period cannot excede one year.

### publication period
The publication period of a disruption is the period on which we want to display the disruption in navitia.

The creator of the disruption might not want the traveller to know of a disruption before a certain date (because it's too uncertain, secret, ...)
The publication period is the way to control this

### application periods
The application periods are the list of periods on which the disruption is active

## query dates

Another thing that can be tricky to understand this that there can be several datetimes for a query

### now

is this document 'now' will represent the actual datetime at which the query is made

# Summary

To sum up we display an impact if 'now' is in the publication period AND the action period intersects the application periods.

The status of the impact depends only of 'now' and is:
* 'active' if 'now' is inside an application periods
* 'future' if there is an application periods after 'now'
* 'past' otherwise



| Navitia Data     | Impact                                 ||  Request date  |              Show impacts                  |||     Status     |||
|Production period | publication period  |Application period |                | [disruptions](#disruptions) API|[traffic_report](#traffic_report) API| [PtRef](#pt-ref) API| [disruptions](#disruptions) API|[traffic_report](#traffic_report) API| [PtRef](#pt-ref) API|
|------------------|:---------------------------------------:|----------------|:--------------------------------------------------------------------:|:----------------------------:|
|                  |                     |                   |                |                |                  |          |                       |                  |           |
|                  |                     |                   |      date1     |    **Yes**     |        -         |    -     |        future         |       -          |    -      |
|                  |                     |                   |                |                |                  |          |                       |                  |           |
|         *        |                     |                   |                |                |                  |          |                       |                  |           |
|        \|        |                     |                   |      date2     |    **Yes**     |       -          |   -      |        future         |       -          |    -      |
|        \|        |         *           |                   |                |                |                  |          |                       |                  |           |
|        \|        |        \|           |                   |      date3     |     **Yes**    |      **Yes**     |**Yes**   |        future         |       future     |future     |
|        \|        |        \|           |         *         |                |                |                  |          |                       |                  |           |
|        \|        |        \|           |        \|         |      date4     |     **Yes**    |      **Yes**     |**Yes**   |        active         |      active      |active     |
|        \|        |        \|           |        \|         |                |                |                  |          |                       |                  |           |
|        \|        |        \|           |         *         |                |                |                  |          |                       |                  |           |
|        \|        |        \|           |                   |      date5     |     **Yes**    |     **Yes**      |**Yes**   |        passed         |     passed       |passed     |
|        \|        |        \|           |                   |                |                |                  |          |                       |                  |           |
|        \|        |         *           |                   |                |                |                  |          |                       |                  |           |
|        \|        |                     |                   |      date6     |      **Yes**   |       -          |   -      |       passed          |        -         |     -     |
|         *        |                     |                   |                |                |                  |          |                       |                  |           |
|                  |                     |                   |      date7     |      **Yes**   |       -          |    -     |       passed          |        -         |      -    |
