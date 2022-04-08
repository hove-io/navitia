<h1 id="places-nearby">Places nearby</h1>

>[Try it on Navitia playground (click on "MAP" buttons to see places)](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%3ARAT%3ASA%3ACAMPO%2Fplaces_nearby&token=3b036afe-0110-4202-b9ed-99718476c2e0)

The Places Nearby feature displays the different transport options around a location - a GPS coordinate,
or an address, for example.

[![a map around on Navitia playground](playground_places_nearby.png)](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fcoord%2F2.32794%253B48.817184%2Fplaces_nearby%3F&token=3b036afe-0110-4202-b9ed-99718476c2e0)

Using OpenStreetMap data, this function also provides information about bicycle parking, public park schedules, and parking fees.

You can use these APIs (click on them for details):

-   **[Coverage](#coverage)**: List of the region covered by navitia
-   **[Public transportation objects](#pt-ref)**: Seek and search within the public transport objects of a region
-   **[Places nearby](#places-nearby-api)**: List of objects near an object or using longitude and latitude
-   **[Stop Schedules](#stop-schedules)**, **[Departures](#departures)** and **[Arrivals](#arrivals)**:
Compute time tables for a given resource
