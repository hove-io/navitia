<a name="isochrones"></a>![Isochrones](/images/isochrons.png)Isochrones
===================================

>[Try it on Navitia playground (click on "MAP" buttons for "wow effect")](http://canaltp.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fisochrones%3Ffrom%3D2.377097%253B48.846905%26boundary_duration%255B%255D%3D600%26boundary_duration%255B%255D%3D1200%26boundary_duration%255B%255D%3D1800%26boundary_duration%255B%255D%3D2400%26boundary_duration%255B%255D%3D3000%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)


Whether using a specific set of coordinates or a general location, you can find places within
your reach and their corresponding travel times, using a variety of transportation options.
You can even specify the maximum amount of time you want to spare on travel and find
the quickest way to reach your destination.

[![a simple isochrone request on Navitia playground](isochrones_example.png)](http://canaltp.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fisochrones%3Ffrom%3D2.377097%3B48.846905%26max_duration%3D2000%26min_duration%3D1000)

Isochrone computing exposes information under two formats:

-   either [Journeys](#journeys) service which provides a list with all the reachable stops from a potential destination
with their respective arrival times, travel times and number of matches. Here is a fiddle example:

<a
    href="https://jsfiddle.net/kisiodigital/x6207t6f/"
    target="_blank"
    class="button button-blue">
    Code it yourself on JSFiddle
</a>

-   or [isochrones](#isochrones_api) service which provides a multi-polygon stream in order to plate colors directly on a map,
or to filter geocoded objects inside the polygon. Here is a fiddle example:

<a
    href="https://jsfiddle.net/kisiodigital/u22zsg9y/"
    target="_blank"
    class="button button-blue">
    Code it yourself on JSFiddle
</a>


You can use these APIs (click on them for details):

-   **[Places](#places)** : autocomplete on geographical data to find the departure and destination points from an input text.
-   **[Journeys](#journeys)** : Compute all journeys from a departure point to every reachable point,
and returns a list of all reachable points, ordered by time to reach.
-   **[isochrones](#isochrones_api)** : Compute all journeys from a departure point to every reachable point,
and returns multiple geoJson ready to be displayed on map ! This service is currently in beta.

