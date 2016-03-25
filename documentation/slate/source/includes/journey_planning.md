<a name="journey_planning"></a>![Journey planning](/images/journeys.png)Journey planning
===============================================

The multi-modal itinerary feature allows you to compute best routes from point A to point B 
using all available means of travel, including: bus, train, subway, bike, public bike, walking, car, etc.
This function returns a roadmap with specific instructions for a route based on available information, 
such as: time of departure and arrival, journey time, possible modes of transport, and walking distance.

In order to compute a journey, you may have to use these APIs (click on them for details):

-   **[Places](#places)** : autocomplete on geographical data to find the departure and destination points from an input text.

| url | Result |
|------------------------------------------------|-------------------------------------|
| `/coverage/places`                             | List of geographical objects        |

-   **[Journeys](#journeys)** : compute journeys from points or coordinates

| url | Result |
|------------------------------------------|-------------------------------------|
| `/journeys`                          | List of journeys from wherever land |
| `/coverage/{region_id}/journeys` | List of journeys on a specific coverage |


