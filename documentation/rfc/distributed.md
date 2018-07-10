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

The previous step will let us build a matrix of stop_points accessible per mode that will be updated with "real"
duration by using the configured streetnetwork engine and it's matrix API.

At this point we can launch the public transport part of the algorithm, we will launch one for each valid combination
of departure mode and arrival mode. To start it we need:
  - the origin matrix
  - the destination matrix
  - the direct path
As soon as we have these three elements the PT journey for a combination will be launched.

Kraken returns journeys that we will need to complete by computing a streetnetwork journey
from the origin to the departure stop_point of the first section and another journeys from the destination
stop_point of the last section to the destination.
Like others operations this is done asynchronously as soon as we get a kraken response and if multiple journeys
use the same fallback journey, it will only be computed one time.

By example for a request like this one: https://api.navitia.io/v1/coverage/fr-idf/journeys?from=stop_area%3AOIF%3ASA%3A59238&to=2.33071%3B48.83890&first_section_mode%5B%5D=bike&first_section_mode%5B%5D=walking&datetime=20180614T113500&last_section_mode%5B%5D=walking&

We will do this:
![graph](../diagrams/distributed.png)

