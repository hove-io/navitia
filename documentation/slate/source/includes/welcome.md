Welcome to navitia.io
=====================

Overview
--------

*navitia.io* is the open API for building cool stuff with transport data.
It provide the following services

* journeys computation
* line schedules
* next departures
* exploration of public transport data / search places
* and sexy things such as isochrones

navitia is a RESTful API that returns JSON formated results.
The API has been built on the [HATEOAS model](http://en.wikipedia.org/wiki/HATEOAS) so it should be quite self explanatory since the interactions are defined in hypermedia.

Our APIs are available at the following url: <https://api.navitia.io/v1> (the latest API version is ``v1``).

Have a look at the examples below to learn what services we provide and how to use them.

Getting started
---------------

#### First,
get a token here <http://navitia.io/register/>

#### Second,
use the token : if you use a web browser, you only have to **paste it in the user area**,
with **no password**. 
Or, in a simplier way, you can add your token in the address bar like :


<aside class="success">
<b>https://01234567-89ab-cdef-0123-456789abcdef@api.navitia.io/v1/coverage/fr-idf/networks</b>
</aside>

See [authentication](#authentication) section to find out more details on **how to use your token**.

#### Then,
use the API ! The easiest is probably to jump to [Examples](#some_examples) below.

At some point you will want to read:

- [transport public lexicon](#lexicon)
- [Ressources overview](#ressources)

About the data
--------------

The street network is extracted from [OpenStreetMap](#http://www.openstreetmap.org). The public transport data are provided by networks that provide their timetables as open data. Some data improvements are achieved by Kisio Digital and are published back there https://navitia.opendatasoft.com/

<aside class="success">
    We are gradually supporting more and more cities. If your city has open public transport data and is missing, drop us a note.
    We will add it to navitia.io
</aside>


Getting help
------------

All available functions are documented in [integration part](#navitia-documentation-v1-interface)

A mailing list is available to ask question: <a href="mailto:navitia@googlegroups.com">navitia@googlegroups.com</a>

In order to report bug and make requests we created a github navitia project
<https://github.com/CanalTP/navitia/issues>.

Stay tuned on twitter @navitia.

At last, we are present on IRC on the network <a href="https://webchat.freenode.net/">Freenode</a>, channel <b>#navitia</b>.

