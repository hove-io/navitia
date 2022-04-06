<h1 id="isochrones">Isochrones</h1>

>[Try it on Navitia playground (click on "MAP" buttons for "wow effect")](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fisochrones%3Ffrom%3D2.377097%253B48.846905%26boundary_duration%255B%255D%3D600%26boundary_duration%255B%255D%3D1200%26boundary_duration%255B%255D%3D1800%26boundary_duration%255B%255D%3D2400%26boundary_duration%255B%255D%3D3000%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

Whether using a specific set of coordinates or a general location, you can find places within
your reach at a given time and their corresponding travel times, using a variety of transportation options.
You can even specify the maximum amount of time you want to spare on travel and find
the quickest way to reach your destination.

[![a simple isochrone request on Navitia playground](isochrones_example.png)](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fisochrones%3Ffrom%3D2.377097%3B48.846905%26max_duration%3D2000%26min_duration%3D1000&token=3b036afe-0110-4202-b9ed-99718476c2e0)

Isochrone computing exposes information under two formats:

-   either [Journeys](#journeys) service which provides a list with all the reachable stops at a given time from a potential destination
with their respective arrival times, travel times and number of matches. Here is a fiddle example:

<a
    href="https://jsfiddle.net/kisiodigital/x6207t6f/"
    target="_blank">
    Code it yourself on JSFiddle
</a>

-   or [isochrones](#isochrones-api) service which provides a multi-polygon stream in order to plate colors directly on a map,
or to filter geocoded objects inside the polygon. Here is a fiddle example:

<a
    href="https://jsfiddle.net/kisiodigital/u22zsg9y/"
    target="_blank">
    Code it yourself on JSFiddle
</a>

You can use these APIs (click on them for details):

-   some **[Places](#places)** requests: autocomplete on geographical data from an input text to find the isochrone starting point.
-   **[Journeys](#journeys)**: Compute all journeys from a departure point at a given time to every reachable point,
and returns a list of all reachable points, ordered by time to reach.
-   **[isochrones](#isochrones-api)**: Compute all journeys from a departure point at a given time to every reachable point,
and returns multiple geoJson ready to be displayed on map. This service is currently in beta.
