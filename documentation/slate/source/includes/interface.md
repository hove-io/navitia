interface
=========

The base URL for **navitia.io** is :
<https://api.navitia.io/v1/>

We aim to implement [HATEOAS](http://en.wikipedia.org/wiki/HATEOAS)
concept with Navitia.

Every resource returns a response containing a links object, a paging
object, and the requested objects, following hypermedia principles.
That's lots of links. Links allow you to know all accessible uris and services for a given point.

Paging
------

<aside class="success">
    The results are paginated to avoid crashing your parser. The parameters to get the next or previous page are within the ``links`` section of the result.
</aside>

Every Navitia response contains a paging object

|Key           |Type|Description                           |
|--------------|----|--------------------------------------|
|items_per_page|int |Number of items per page              |
|items_on_page |int |Number of items on this page          |
|start_page    |int |The page number                       |
|total_result  |int |Total number of items for this request|

You can navigate through a response using 2 parameters

|Parameter |Type|Description              |
|----------|----|-------------------------|
|start_page|int |The page number          |
|count     |int |Number of items per page |

<aside class="notice">
    The number of objects returned for a request can <b>not be superior than 1000</b>. 
    If you request for more, Navitia will return the first 1000, and you will have to paginate to get next 1000.
</aside>


Templated URL
-------------

> From every object collection

``` shell
#first request

$ curl 'https://api.navitia.io/v1/coverage/sandbox/lines' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#response
HTTP/1.1 200 OK

#first line is like:

{
    "lines": [
        {
            "id": "line:RAT:M1",
            "code": "1",
            "name": "Château de Vincennes - La Défense",
            "...": "..."
        },
        {...}
        ],
    ...,
 
#and a templated link from the example above:

    "links": [
        {
            "href": "https://api.navitia.io/v1/coverage/sandbox/lines/{lines.id}/stop_schedules",
            "rel": "stop_schedules",
            "templated": true
        },
        {...}
    ]
}

#you can then request for "stop_schedules" service using templating
#be careful, without any filter, the response can be huge

#second request
#{line.id} has to be replaced by "line:RAT:M1"
$ curl 'https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/stop_schedules' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

#here is a smarter request for a line AND a stop_area
$ curl 'https://api.navitia.io/v1/coverage/sandbox/lines/line:RAT:M1/stop_areas/stop_area:RAT:SA:PLROY/stop_schedules' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'

```

Under some link sections, you will find a "templated" property.

If "templated" is true, then you will have to format the link with your right id as describe in the example.
In order to do that, you will have to 

* take the id from the object you want to get the linked service
* replace {lines.id} in the url as the example



Inner references
----------------
``` shell
#You may find "disruptions" link, as

{
    "internal": true,
    "type": "disruption",
    "id": "edc46f3a-ad3d-11e4-a5e1-005056a44da2",
    "rel": "disruptions",
    "templated": false
}
```


Some link sections holds disruption links. These links are templated.

That means :

* inside the self stream ( **"internal": true** ) 
* you will find a **disruptions** section ( **"rel": "disruptions"** ) 
* containing some [disruptions](#disruption) objects ( **"type": "disruption"** ) 
* where you can find the details of your object ( **"id": "edc46f3a-ad3d-11e4-a5e1-005056a44da2"** ).


