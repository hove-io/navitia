Scenario Distributed
====================


This scenario allows us to call multiple engines for computing fallbacks. These engines can be:

  - kraken
  - geovelo
  - here
  - valhalla

The orchestration is a bit complex, so this documentation will try to explain what is done, and in what order.
Each fallback mode can use a specific engine that is configured at the instance level.

First we search for the origin and destination objects with kraken.

Then we compute for each departure fallback modes the direct path between the origin and the destination with the
appropriate engine. This is done asynchronously.
If public transport is disabled (max_duration=0) jormungandr returns these results and stops.

Then we start computing the stop points that can be reached for each mode at both extremity. 
This is done by using a crowfly at constant speed that is computed by kraken.

While this is computing we look for stop point that can be accessed in zero sec (typically the stop_point
of the stop_area). This might require a call to kraken in some cases.

At this point we have to wait for all of the previous tasks to finish, when this is done we can create a matrix of
stop_points accessible per mode, that we update with "real" duration by using the configured engine for each mode.
This is where we use the matrix capabilities of the streetnetwork engines.

After this we have for each modes at the origin and the destination a matrix of stop_points with the time required to access them.

This matrix is used to start the Public Transport part of the algorithm.
Kraken returns journeys that we will need to complete by computing a streetnetwork journey
from the origin to the departure stop_point of the first section and another journeys from the destination
stop_point of the last section to the destination.
