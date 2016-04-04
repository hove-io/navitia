interface
=========

The only endpoint of this version of the api is :
<https://api.navitia.io/v1/>

We aim to implement [HATEOAS](http://en.wikipedia.org/wiki/HATEOAS)
concept with Navitia.

All the resources return a response containing a links object, a paging
object, and the requested objects. That's lots of links. Links allow
you to know all accessible uris and services for a given point.

Paging
------

<aside class="success">
    The results are paginated to avoid crashing your parser. The parameters to get the next or previous page are within the ``links`` section of the result.
</aside>

All response contains a paging object

|Key           |Type|Description                           |
|--------------|----|--------------------------------------|
|items_per_page|int |Number of items per page              |
|items_on_page |int |Number of items on this page          |
|start_page    |int |The page number                       |
|total_result  |int |Total number of items for this request|

You can navigate through a request with 2 parameters

|Parameter |Type|Description              |
|----------|----|-------------------------|
|start_page|int |The page number          |
|count     |int |Number of items per page |

<aside class="notice">
    The number of objects returned for a request can <b>not be superior than 1000</b>. 
    If you request for more, Navitia will return the first 1000, and you will have to paginate to get next 1000.
</aside>


Templated url
-------------

Under some link sections, you will find a "templated" property. If
"templated" is true, then you will have to format the link with one id.

For example, in response of
<https://api.navitia.io/v1/coverage/fr-idf/lines> you will find a
*links* section:

``` json
{
    "href": "https://api.navitia.io/v1/coverage/fr-idf/lines/{lines.id}/stop_schedules",
    "rel": "route_schedules",
    "templated": true
}
```

You have to put one line id instead of *{lines.id}*. For example:
<https://api.navitia.io/v1/coverage/fr-idf/lines/line:OIF:043043001:N1OIF658/stop_schedules>

Inner references
----------------

Some link sections look like

``` json
{
    "internal": true,
    "type": "disruption",
    "id": "edc46f3a-ad3d-11e4-a5e1-005056a44da2",
    "rel": "disruptions",
    "templated": false
}
```

That means you will find inside the same stream ( *"internal": true* ) a
*disruptions* section ( *"rel": "disruptions"* ) containing some
[disruptions](#disruption) objects ( *"type": "disruption"* ) where you can find the
details of your object ( *"id": "edc46f3a-ad3d-11e4-a5e1-005056a44da2"*
).


