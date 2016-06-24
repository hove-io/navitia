<a name="some_examples"></a>Some examples
=========================================

This chapter shows some usages with the minimal required arguments. However, this is not a reference and not all APIs nor arguments are shown.

<aside class="notice">
You will have to use your own token with the examples below.
</aside>


A quick exploration
-------------------
``` shell
$ curl 'https://api.navitia.io/v1/coverage' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'


HTTP/1.1 200 OK

{
    "start_production_date": "20140105",
    "status": "running",
    "shape": "POLYGON((-74.500997 40.344999,-74.500997 41.096999,-73.226 41.096999,-73.226 40.344999,-74.500997 40.344999))",
    "id": "sandbox",
    "end_production_date": "20140406"
}
```

*navitia* allows to dive into the public transport data.

To better understand how the API works let's ask the API the different main possibilities by simply querying the API endpoint: <https://api.navitia.io/v1/>

The ``links`` section of the answer contains the different possible interactions with the API.

As you can see there are several possibilities like for example [coverage](#coverage) to navigate through the covered regions data or [journeys](#journeys) to compute a journey.


Now let's see what interactions are possible with ``coverage``:

This request will give you:

* in the ``regions`` section the list of covered regions
* in the ``links`` section the list of possible interactions with them


``` shell
$ curl 'https://api.navitia.io/v1/coverage/sandbox/lines' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'


HTTP/1.1 200 OK

{
    "lines":[
        {
            "id":"line:RAT:M1",
            "code":"1",
            "name":"Château de Vincennes - La Défense",
            "opening_time":"053000",
            "closing_time":"013600",
            "color":"F2C931",
            "commercial_mode":{
                "id":"commercial_mode:Metro",
                "name":"Metro"
            },
            "physical_modes":[
                {
                    "id":"physical_mode:Metro",
                    "name":"Métro"
                }
            ],
            "network":{
                "id":"network:RAT:1",
                "name":"RATP"
            },
            "routes":[
                {
                    "id":"route:RAT:M1",
                    "name":"Château de Vincennes - La Défense",
                    "direction":{
                        "id":"stop_area:RAT:SA:DENFE",
                        "name":"La Défense Grande Arche (Puteaux)"
                    }
                },
                {
                    "id":"route:RAT:M12_R",
                    "name":"Mairie d'Issy - Front Populaire",
                    "direction":{
                        "id":"stop_area:RAT:SA:MISSY",
                        "name":"Mairie d'Issy (Vanves)"
                    }
                }
            ]
        }
    ]
}
```


In the ``links`` section there is for example this link: ``"href": "https://api.navitia.io/v1/coverage/{regions.id}/lines"``


This link is about lines (according to its ``rel`` attribute) and is templated which means that it needs additional parameters.<br>
The parameters are identified with the ``{`` ``}`` syntax.
In this case it needs a region id. This id can the found in the ``regions`` section. 

To query for the public transport lines of New York we thus have to call: <https://api.navitia.io/v1/coverage/us-ny/lines>


Easy isn't it?


<a
    href="http://jsfiddle.net/gh/get/jquery/2.2.2/CanalTP/navitia/tree/documentation/slate/source/examples/jsFiddle/lines/"
    target="_blank"
    class="button button-blue">
    Code it yourself on JSFiddle
</a>


We could push the exploration further and:

- Where am I? (WGS 84 coordinates)
    - <https://api.navitia.io/v1/coord/2.377310;48.847002>
    - I'm on the "/fr-idf" coverage, at "20, rue Hector Malot in Paris, France"
- Services available on this coverage
    - <https://api.navitia.io/v1/coverage/fr-idf>
    - Let's take a look at the links at the bottom of the previous stream
- Networks available? (see what [network](#network) is)
    - <https://api.navitia.io/v1/coverage/fr-idf/networks>
    - pwooo, many networks on this coverage ;)
- Is there any Metro lines or networks?
    - there is an api for that. See [pt_objects](#pt-objects)
    - <https://api.navitia.io/v1/coverage/fr-idf/pt_objects?q=metro>
    - Response contain one network, one mode, and many lines
- Let's try some filtering (see [PT objects exploration](#pt-ref))
    - filter on the specific metro network ("id": "network:OIF:439" extracted from last request)
    - <https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/>
    - physical modes managed by this network
    - <https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/physical_modes>
    - metro lines
    - <https://api.navitia.io/v1/coverage/fr-idf/networks/network:OIF:439/physical_modes/physical_mode:Metro/lines>
- By the way, what stuff are close to me?
    - <https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/places_nearby>
    - or <https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/lines>
    - or <https://api.navitia.io/v1/coverage/fr-idf/coords/2.377310;48.847002/stop_schedules>
    - or ...

Seek and search
---------------

### What places have a name that start with 'eiff'


The [/places](#places) API finds any object whose name matches the first letters of the query.

To find the objects that start with "tran" the request should be: <https://api.navitia.io/v1/coverage/fr-idf/places?q=eiff>

This API is fast enough to use it for autocompleting a user request.


### What places are within 1000 meters


The [/places_nearby](#places-nearby) API finds any object within a certain radius as a crow flies.
This API is not accessible from the main endpoint but has to be applied on a stop point, an address, some coordinates,...

All objects around the coordinates of the Transamerica Pyramid can be fetched with the following request: <https://api.navitia.io/v1/coverage/us-ca/coords/-122.402770;37.794682/places_nearby>

We could, in the same fashion, ask for the objects around a particuliar stop area (``stop_area:OSF:SA:CTP4025`` for example): <https://api.navitia.io/v1/coverage/us-ca/stop_areas/stop_area:OSF:SA:CTP4025/places_nearby>

Optionally you can select what object types to return and change the radius.

About itinerary
---------------

### A simple route computation

Let's find out how to get from the view point of the Golden Gate bridge to the Transamerica Pyramid in San Francisco.
We need to use the ``journeys`` API.


The coordinates of the view point are ``longitude = -122.4752``, ``latitude = 37.80826`` and the coordinates of the Transamercia Pyramid are ``longitude = -122.402770``, ``latitude = 37.794682``.
The coordinates are always in decimal degrees as WGS84 (also known as GPS coordinates). The coordinates are given to the API with the following format: ``longitute;latitude``.

The arguments are the following:


* ``from=-122.4752;37.80826``
* ``to=-122.402770;37.794682``
Hence, the complete URL: <https://api.navitia.io/v1/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682>.

<aside class="success">
A ``journeys`` request might return multiple journeys. Those journeys are said to be equivalent. For instance
a journey can be faster than an other but requires more changes or more walking.
</aside>

This API has more options explained in the reference as:

* The dates are given in the basic form of the ISO 8601 datetime format: ``YYYYMMDDTHHMM``. 
  For example, if you want to compute a journey on friday, April 22 ``datetime=20160422T0800``
  <https://api.navitia.io/v1/journeys?from=-122.47787733594924;37.71696489300146&to=-122.41539259473815;37.78564348914185&datetime=20160422T0800>

* Latest departure
  To get the latest departure, you can query for some journeys arriving before the end of the service
  <https://api.navitia.io/v1/journeys?from=-122.47787733594924;37.71696489300146&to=-122.41539259473815;37.78564348914185&datetime=20160422T2359&datetime_represents=arrival>

* You can also change the [traveler profile](#traveler-type) (to adapt the walking/biking/driving parts and comfort of journeys):
  <https://api.navitia.io/v1/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682&traveler_type=slow_walker>

* Forbid certain lines, routes or modes
  For example you can forbid the line 1 and the cable car mode with the url:
  <https://api.navitia.io/v1/journeys?from=-122.4752;37.80826&to=-122.402770;37.794682&forbidden_uris[]=line:OSF:10867&forbidden_uris[]=commercial_mode:cablecar>

* Enable biking, driving or use of bike sharing system on Paris area
  For example you can allow bss (and walking since it's implicitly allowed with bss) at the departure:
  <https://api.navitia.io/v1/journeys?from=2.3865494;48.8499182&to=2.3643739;48.854&first_section_mode[]=bss&first_section_mode[]=walking&first_section_mode[]=bike>


### What stations can be reached in the next 20 minutes

The API can computes *all* the reachable stop points from an origin within a given maximum travel duration.
That's what we call an `isochron` (see [journeys section](#journeys) )

All the stop points that can be reached from the Transamerica Pyramid can be fetched with the following request:
<https://api.navitia.io/v1/coverage/us-ca/coords/-122.402770;37.794682/journeys?max_duration=1200>

It returns for each destination stop point the earliest arrival and a link to the journey detail.


