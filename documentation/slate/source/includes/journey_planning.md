<h1 id="journey_planning">Journey planning</h1>

>[Try it on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fjourneys%3Ffrom%3D2.3749036%3B48.8467927%26to%3D2.2922926%3B48.8583736&token=3b036afe-0110-4202-b9ed-99718476c2e0)

The multi-modal itinerary feature allows you to compute the best routes from point A to point B
using all available means of travel, including: bus, train, subway, bike, public bike, walking, car, etc.
This function returns a roadmap with specific instructions for a route based on available information,
such as: time of departure and arrival, journey time, possible modes of transport, and walking distance.

[![a journey on Navitia playground](playground_journey.png)](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fjourneys%3Ffrom%3D2.3749036%253B48.8467927%26to%3D2.2922926%253B48.8583736%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

In order to compute a journey, you may have to use these APIs (click on them for details):

-   **[Places](#places)**: autocomplete on geographical data to find the departure and destination points from an input text.
-   **[Journeys](#journeys)**: compute journeys from and to coordinates, stops, stations or administrative region
