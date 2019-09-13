# Frequency Vehicle Journeys

Frequencies vehicle journeys are a special case of vehicle journeys as they operate multiple time per day
contrary to discrete vehicle journeys that operate only one time per day.
They are used to define a journey template that will start its mission at a defined interval for a
specified time range.
By example a journey that serves A, B and C, with a new VJ starting from A every five minutes between 06h00 and
21h00.

They are quite an exception as we know very few agencies that model their networks with frequencies, but we use
them to model some kind of on demand transport.

## Input data feed

The GTFS specification refers to [`frequencies.txt`](https://gtfs.org/reference/static#frequenciestxt)
that represents trips which operate on regular headways (time between trips). The NTFS is similar on that point so no distinction
between them will be made in this documentation.

`frequencies.txt` file is read by *fusio2ed*. For each line, the corresponding vehicle journey is updated with
*start_time*, *end_time* and *headway_secs*.

## navitia model

On the kraken side, frequencies vehicle journeys are implemented as a sub class of `VehicleJourney` named
`FrequencyVehicleJourney` that adds the fields *start_time*, *end_time* and *headway_secs* to a regular vehicle
journey. They share the same `StopTime` class but use it differently: in a `DiscreteVehicleJourney` the stop
time contains the passage time from midnight while in a `FrequencyVehicleJourney` the passage time is the shift
from the start of *this* vehicle journey.

All of this is abstracted by `get_next_stop_time` to make raptor handle both type of vehicle journeys the same
way.

## Example

*frequencies.txt*

| trip_id | start_time | end_time | headway_secs |
| ------- | ---------- | -------- | ------------ |
| 1       | 05:00:00   | 05:30:00 | 600          |


*stop_times.txt*

| trip_id | arrival_time | derparture_time | stop_id | stop_sequence |
| ------- | ------------ | --------------- | ------- | ------------- |
| 1       | 05:00:00     | 05:00:00        | A       | 1             |
| 1       | 05:10:00     | 05:10:00        | B       | 2             |
| 1       | 05:20:00     | 05:20:00        | C       | 3             |

*Schedules of this trip*

|     A    |    B     |    C     |
| -------- | -------- | -------- |
| 05:00:00 | 05:10:00 | 05:20:00 |
| 05:10:00 | 05:20:00 | 05:30:00 |
| 05:20:00 | 05:30:00 | 05:40:00 |
| 05:30:00 | 05:40:00 | 05:50:00 |


*Generated stop times list*

| stop | arrival_time | departure_time |
| ---- | ------------ | -------------- |
| A    | 00:00:00     | 00:00:00       |
| B    | 00:10:00     | 00:10:00       |
| C    | 00:20:00     | 00:20:00       |

