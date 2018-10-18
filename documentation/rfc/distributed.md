Scenario Distributed
====================


This scenario allows us to call multiple engines for computing fallbacks. These engines can be:

  - kraken
  - geovelo
  - here
  - valhalla

The orchestration is a bit complex, so this documentation will try to explain what is done, and in what order.
Each fallback mode can use a specific engine that is configured at the instance level.

1. First we search for the origin and destination objects with kraken.

2. Then, for each departure fallback mode, we compute the direct path between the origin and the destination with the appropriate engine. This is done asynchronously.
If public transport is disabled (max_duration=0) jormungandr returns these results and stops.

3. Then, we start computing the stop points that can be reached for each mode at both extremities.
This is computed by kraken using a crowfly at constant speed.

4. While this is computing, we look for stop points that can be accessed in zero sec (typically the stop_points
of a stop_area). This might require a call to kraken in some cases.

5. The previous step will let us build a matrix of stop_points accessible per mode that will be updated with "real"
durations by using the configured streetnetwork engine and its matrix API.

6. At this point, we can start the public transport part of the algorithm. We will trigger the computation of each valid combination
of departure mode and arrival mode. To start it, we need:
  - the origin matrix
  - the destination matrix
  - the direct path

7. We wait on Kraken to perform all the computation, and recall step 6. if we need more journeys.

8. Now we have the public transport journeys and the stop_point's matrix fallback timings (6. and 5.), so we can build temporary crow-flies fallbacks foreach journeys. The crow-flies will have a realistic fallback durations but without the actual details (to be computed later on by the street-network service).
With an accurate approximation of the end-to-end journeys' durations, we ca apply all the different filters (ie. long waiting, min transfert, direct path etc...).
 ```
 Origin     <tmp_crow_fly> 1st stop_point <public_transport> last stop_point <tmp_crow_fly> Destination
 19 rue Hec <----5min---->  Gare de Lyon  <-----M1 4min---->  Hotel de Ville <----2min----> Notre-Dame
```

9. Finaly, we can complet the fallback details from the list of filtered journeys by replacing the temporary crow-fly with the detailed street network. We need to compute:
 - a streetnetwork journey from the origin to the departure's stop_point of the first section
 - another journey from the destination's stop_point of the last section to the destination
```
 Origin     <street_network> 1st stop_point <public_transport> last stop_point <street_network> Destination
 19 rue Hec <--walk 5min---> Gare de Lyon   <-----M1 4min----> Hotel de Ville  <--bike 2min---> Notre-Dame
```

Like others operations, this is done asynchronously as soon as we get a response from Kraken. If multiple journeys
use the same fallback journey, it will only be computed one time.

For instance, a request like—https://api.navitia.io/v1/coverage/fr-idf/journeys?from=stop_area%3AOIF%3ASA%3A59238&to=2.33071%3B48.83890&first_section_mode%5B%5D=bike&first_section_mode%5B%5D=walking&datetime=20180614T113500&last_section_mode%5B%5D=walking&, will produce the following calls.

Dependency graph:

![graph](../diagrams/distributed.png)

Sequence diagram:

![sequence](../diagrams/Distributed.svg)
