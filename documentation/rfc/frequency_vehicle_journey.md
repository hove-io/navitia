# Frequency Vehicle Journeys

The GTFS specification refers to [`frequencies.txt`](https://gtfs.org/reference/static#frequenciestxt)
that represents trips wich operate on regular headways.<br/>
This RFC describes how kraken works with it.

In first step, `frequencies.txt` file is readed by *fusio2ed*. For each line, a stop time list is created from
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
Vehicle journeys data structure to discriminate it. When starting *Kraken*, this data is loaded inside Raptor cache.

Furthermore, if the *trip_id* is the same, frequency parameters impact the given stop times, listed inside
[`stop_times.txt`](https://gtfs.org/reference/static/#stop_timestxt).<br/>
Only the *start_time* parameters is used to shift each stop time in the list. Example:

*frequencies.txt*

| trip_id | start_time | end_time | headway_secs |
| ------- | ---------- | -------- | ------------ |
| 1       | 05:00:00   | 15:00:00 | 600          |

*stop_times.txt*

| trip_id | arrival_time | departure_time | stop_id | stop_sequence | stop_headsign | pickup_type | drop_off_type | shape_dist_traveled |
| ------- | ------------ | -------------- | ------- | ------------- | ------------- | ----------- | ------------- | ------------------- |
| 1       | 09:10:00     | 09:10:00       |         |               |               |             |               |                     |
| 1       | 09:20:00     | 09:20:00       |         |               |               |             |               |                     |
| 1       | 09:30:00     | 09:30:00       |         |               |               |             |               |                     |
| 1       | 09:40:00     | 09:40:00       |         |               |               |             |               |                     |
| 1       | 09:50:00     | 09:50:00       |         |               |               |             |               |                     |
| 1       | 10:00:00     | 10:00:00       |         |               |               |             |               |                     |

*Generated stop times list*

| arrival_time | departure_time |
| ------------ | -------------- |
| 05:00:00     | 05:00:00       |
| 05:10:00     | 05:10:00       |
| 05:20:00     | 05:20:00       |
| 05:30:00     | 05:30:00       |
| 05:40:00     | 05:30:00       |
| 05:50:00     | 05:30:00       |
| 10:00:00     | 10:00:00       |

The consequence of this meld should be visible with the `/vehicle_journeys` API that exposes the shifted list.

