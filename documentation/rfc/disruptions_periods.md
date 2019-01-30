# Context

There are several periods in the disruptions model and they can be tricky to understand so here is a summary over them.

## Periods

### production period 
The production period is global to a navitia coverage and is the validity period of the coverage's data.  
This production period cannot exceed one year.

### publication period
The publication period of a disruption is the period on which we want to display the disruption in navitia.

The creator of the disruption might not want the traveller to know of a disruption before a certain date (because it's too uncertain, secret, ...).  
The publication period is the way to control this.

Please note that this period can be out of production period, but shouldn't affect impact's  management.

### application periods
The application periods is the list of periods on which the disruption is active.

Please note that these periods can be out of production period, but shouldn't affect impact's  management.

## Query dates

Another thing that can be tricky to understand this that there can be several datetimes for a query.


### now

In this document 'now' will represent the actual datetime at which the query is made.

### action period

For some endpoints (called 'contextual apis' in the following document) we query navitia on a certain date.

The easiest way to understand this is a user that query navitia to plan a journey.

The user calls navitia the 1st on january at 10:00 for a journey the 4th of january at 12:00.

In this example 'now' is 01/01 at 10:00, 'action period' is, for each section of the computed journeys, the period on which the user will have to be in the bus.

### filter period

On some ptref endpoints, one can provide a 'since' and 'until' parameter to filter disruptions or vehicle_journeys.  
Please refer to official navitia's API doc for use.


# Summary

To sum up we display an impact if 'now' is in the publication period AND the action period intersects the application periods.

The status of the impact depends only of 'now' and is:
* `active` if 'now' is inside one of the application periods
* otherwise `future` if there is an application periods after 'now'
* `past` otherwise


# Examples

To understand a bit how those periods behave with the different query dates let's consider this example:

```
[-------------------------------------------------------------]                         production period
                   
                                                Impact
                                    <--------------------------------->                 publication period
                   
                                                     (------------)                     application period
                   
                   
```

## now is before the impact's publication period

```
[------------------|------------------------------------------]                         production period
                   |
                   |                             Impact
                   |                 <----------------------------->                    publication period
                   |
                   |                                  (------------)                    application period
                   |
                   |
                  now

```

### ptref

do we show the impact ?      No 

### contextual api: /journeys, /stop_schedules

do we show the impact ?      No 


## now is inside the impact's publication period but before the application period

```
[------------------------------------------|-----------------------------------]        production period
                                           |                                           
                                           |     Impact
                                    <------|----------------------->                    publication period
                                           |                                           
                                           |          (------------)                    application period
                                           |                                           
                                          now                                           

```
### ptref
do we show the impact ?      Yes

what is it's status ?        futur

### contextual api: /journeys, /stop_schedules

#### context is before the application period

```
[------------------------------------------|------------------------------------]       production period
                                           |                                           
                                           |     Impact
                                    <------|----------------------->                    publication period
                                           |                                           
                                           |          (------------)                    application period
                                           |                                           
                                          now                                           
                                 {----}
                              action period

```
do we show the impact ?      No

what is it's status ?        -

#### context intersects the application period

```
[------------------------------------------|-----------------------------------]        production period
                                           |                                           
                                           |     Impact
                                    <------|----------------------->                    publication period
                                           |                                           
                                           |          (------------)                    application period
                                           |                                           
                                          now                                           
                                                   {----}
                                               action period

```
do we show the impact ?      Yes

what is it's status ?        futur 

#### context is after the application period

```
[------------------------------------------|-----------------------------------]        production period
                                           |                                           
                                           |     Impact
                                    <------|----------------------->                    publication period
                                           |                                           
                                           |          (------------)                    application period
                                           |                                           
                                          now                                           
                                                                       {----}
                                                                   action period

```
do we show the impact ?      No

what is it's status ?        - 


## now is inside the impact's publication period and inside the application period

```
[-----------------------------------------------------------|------------------]        production period
                                                            |                          
                                                Impact      |                         
                                    <-----------------------|------>                    publication period
                                                            |                          
                                                     (------|------)                    application period
                                                            |                          
                                                           now                          


```
### ptref
do we show the impact ?      Yes

what is it's status ?        Active

### contextual api: /journeys, /stop_schedules

#### context is before the application period

```
[-----------------------------------------------------------|------------------]        production period
                                                            |                          
                                                Impact      |                         
                                    <-----------------------|------>                    publication period
                                                            |                          
                                                     (------|------)                    application period
                                                            |                          
                                                           now                          
                                        {----}
                                     action period

```
do we show the impact ?      No

what is it's status ?        -

#### context intersects the application period

```
[-----------------------------------------------------------|------------------]        production period
                                                            |                          
                                                Impact      |                         
                                    <-----------------------|------>                    publication period
                                                            |                          
                                                     (------|------)                    application period
                                                            |                          
                                                           now                          
                                                                {----}
                                                            action period

```
do we show the impact ?      Yes

what is it's status ?        Active

#### context is after the application period

```
[-----------------------------------------------------------|------------------]        production period
                                                            |                          
                                                Impact      |                         
                                    <-----------------------|------>                    publication period
                                                            |                          
                                                     (------|------)                    application period
                                                            |                          
                                                           now                          
                                                                      {----}
                                                                  action period

```
do we show the impact ?      No

what is it's status ?        -

