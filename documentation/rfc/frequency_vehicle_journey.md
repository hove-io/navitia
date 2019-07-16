# Frequency Vehicle Journeys

The GTFS specification refers to [`frequencies.txt`](https://gtfs.org/reference/static#frequenciestxt)
that represents trips which operate on regular headways.<br/>
This RFC describes how kraken works with it.

At first, `frequencies.txt` file is read by *fusio2ed*. For each line, a stop time list is created from
*start_time* to *end_time* with a step of *headway_secs* for a given `trip_id`. Example:

*frequencies.txt*

| trip_id | start_time | end_time | headway_secs |
| ------- | ---------- | -------- | ------------ |
| 1       | 05:00:00   | 10:00:00 | 600          |

*Generated stop times list*

| arrival_time | departure_time |
| ------------ | -------------- |
| 05:00:00     | 05:00:00       |
| 05:10:00     | 05:10:00       |
| 05:20:00     | 05:20:00       |
| 05:30:00     | 05:30:00       |
| ...          | ...            |
| 10:00:00     | 10:00:00       |


The generated stop times list is used to create a `Frequency Vehicle journeys`. A bool flag (`is_frequency`) is available inside
Vehicle journeys data structure to discriminate it.<br/>
When starting *Kraken*, this data is only loaded inside the *Raptor* cache.
This means that it is not append into *stop times* list contained inside *Vehicle Journeys*.

*Frequency Vehicle journey* is not cunning with *classic stop times*.<br/>
As a reminder, stop times are contained inside [`stop_times.txt`](https://gtfs.org/reference/static/#stop_timestxt).<br/>
If the *trip_id* is the same, frequency parameters alter the given stop times, describ into *stop_times.txt* file.
The consequence of this meld makes the list become obsolete. The values are no longer valid and become not consistent.<br/>
For instance, with `/vehicle_journeys` API, we can glimpse the altered stop times list which presents `departure_time/arrival_time` around midnight.<br/>
Nonetheless, if *start_time/end_time/headways_secs* fields are present within the response, it gives you information that it is a *Frequency Vehicle Journey*.
So don't take into account stop times list.

To avoid any problems, the principle of use is the following:
* Don't use *frequencies.txt* and a stop times list (inside *stop_times.txt*) with the same *trip_id*. It does not work well together

