#Errors

Code 40x
--------
> When there's an error you'll receive a response with a error object
containing a unique error id

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

This errors appears when there is an error in the request

The are two possible 40x http codes :

-   Code 404:

| Error id                     | Description                                                                |
|------------------------------|----------------------------------------------------------------------------|
| date_out_of_bounds        | When the given date is out of bounds of the production dates of the region |
| no_origin                   | Couldn't find an origin for the journeys                                   |
| no_destination              | Couldn't find an destination for the journeys                              |
| no_origin_nor_destination | Couldn't find an origin nor a destination for the journeys                 |
| unknown_object              | As it's said                                                               |

-   Code 400:

| Error id          | Description                             |
|-------------------|-----------------------------------------|
| bad_filter       | When you use a custom filter            |
| unable_to_parse | When you use a mal-formed custom filter |

Code 50x
--------

Ouch. Technical issue :/

