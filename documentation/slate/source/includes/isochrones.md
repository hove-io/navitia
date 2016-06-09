<a name="isochrones"></a>![Isochrones](/images/isochrons.png)Isochrones
===================================

Whether using a specific set of coordinates or a general location, you can find places within 
your reach and their corresponding travel times, using a variety of transportation options. 
You can even specify the maximum amount of time you want to spare on travel and find 
the quickest way to reach your destination. 
The Isochrone function provides information in the form of a table with all the possible stops 
from a potential destination with their respective arrival times, travel times and number of matches.

<a
    href="http://jsfiddle.net/gh/get/jquery/2.2.2/CanalTP/navitia/tree/documentation/slate/source/examples/jsFiddle/isochron/"
    target="_blank"
    class="button button-blue">
    Code it yourself on JSFiddle
</a>

You have to use these APIs (click on them for details):

-   **[Places](#places)** : autocomplete on geographical data to find the departure and destination points from an input text.
-   **[Journeys](#journeys)** : Compute all journeys from a departure point to every reachable points
-   **[isochrones](#isochrones)** : same as /journeys, but it returns a geoJson ready to be displayed on map ! This service is currently in beta.

