<a name="next_departures_and_arrivals"></a>![Departures](/images/departures.png)Next departures and arrivals
=======================================================================

The "Next Departures" feature provides the next scheduled departures or arrivals 
for a specific mode of public transport (bus, tram, metro, train) at your selected stop, near coordinates, etc. 

>[Try it on Navitia playground (click on "EXT" buttons to see times)](http://canaltp.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%253ARAT%253ASA%253AGDLYO%2Fstop_schedules%3Fitems_per_schedule%3D2%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

* use `stop_schedules` if you want to display departures grouped by route (2 next departures for each route for example)
[![a stop_schedules on Navitia playground](https://upload.wikimedia.org/wikipedia/commons/thumb/7/7d/Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg/640px-Panneau_SIEL_couleurs_Paris-Op%C3%A9ra.jpg)](http://canaltp.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%253ARAT%253ASA%253AGDLYO%2Fstop_schedules%3Fitems_per_schedule%3D2%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

>[Try it on Navitia playground](http://canaltp.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%253ARAT%253ASA%253AGDLYO%2Fdepartures%3F&token=3b036afe-0110-4202-b9ed-99718476c2e0)

* use `departures` or `arrivals` if you want to display a multi-route table (like big departure boards in train stations)
[![a departures on Navitia playground](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/Display_at_bus_stop_sign_in_Karlovo_n%C3%A1m%C4%9Bst%C3%AD%2C_T%C5%99eb%C3%AD%C4%8D%2C_T%C5%99eb%C3%AD%C4%8D_District.JPG/640px-Display_at_bus_stop_sign_in_Karlovo_n%C3%A1m%C4%9Bst%C3%AD%2C_T%C5%99eb%C3%AD%C4%8D%2C_T%C5%99eb%C3%AD%C4%8D_District.JPG)](http://canaltp.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fstop_areas%2Fstop_area%253ARAT%253ASA%253AGDLYO%2Fdepartures%3F&token=3b036afe-0110-4202-b9ed-99718476c2e0)

In order to display departures, you may have to use these APIs (click on them for details):

-   **[Public transportation objects](#pt-ref)** : List of the public transport
    objects of a region
-   **[Stop Schedules](#stop-schedules)**, **[Departures](#departures)** and **[Arrivals](#arrivals)** : 
Compute time tables for a given resource



