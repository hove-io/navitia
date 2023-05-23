# Errors

Code 40x
--------
When Navitia is unable to give an answer, you'll receive a response with an error object
containing a unique error id, in a 4XX http code response

```shell
#request
$ curl 'https://api.navitia.io/v1/coverage/sandbox/stop_areas/wrong-one' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 404 OK

{
    "error": {
        "id": "bad_filter",
        "message": "ptref : Filters: Unable to find object"
    }
}
```

The are two possible 40x http codes:

-   Code 404: unable to find an object. 

| Error id                    | Description                                                                	|
|-----------------------------|-----------------------------------------------------------------------------|
| date_out_of_bounds          | When the given date is out of bounds of the production dates of the region 	|
| no_departure_this_day       | Some vehicles stop at this point, but none today (for example, it's sunday) |
| no_active_circulation_this_day | No more vehicles stop for today (for example, it's too late)             |
| terminus                    | There will never be departures, you're at the terminus of the line          |
| partial_terminus            | Same as terminus, but be careful, some vehicles are departing from the stop some other days  |
| active_disruption           | No departure, due to a disruption                                           |
| no_origin                   | Couldn't find an origin (for the journey service only)                      |
| no_destination              | Couldn't find a destination (for the journey service only)                  |
| no_origin_nor_destination   | Couldn't find neither origin nor destination (for the journey service only) |
| unknown_object              | Couldn't find one of the request parameters. It can be the region, the API or a PT object |

-   Code 400: bad request

| Error id          | Description                                                 |
|-------------------|-------------------------------------------------------------|
| bad_filter        | When you use a custom filter                                |
| unable_to_parse   | When you use a mal-formed custom filter                     |
| unknown_api       | When you request an inexistant endpoint                     |
| bad_format        | When you provide ill-formated parameter (coord for example) |
| config_exception  | When you try to configure an external service (street network back-end or autocomplete) with wrong parameters |


Code 200
--------
Even if everything is correct (input parameters, internal processing)
Navitia can end up finding that the correct answer is empty.

| Error id                    | Description                                                                |
|-----------------------------|----------------------------------------------------------------------------|
| no_solution                 | From data, there is no solution available given the request                |


Code 50x
--------

Ouch. Technical issue :/

| Error id            	| Description                                               				|
|-----------------------|---------------------------------------------------------------------------|
| internal_error	    | Something bad and unexpected has happened internally      				|
| service_unavailable	| Navitia isn't responding								                    |
| dead_socket		    | The requested region (coverage) isn't responding							|
| technical_error	    | An internal service (street network back-end or autocomplete)	isn't responding	|

