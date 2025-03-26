Getting started
=====================

Overview
--------

**Navitia** (_pronounce [navi-sia]_) is the open API for building cool stuff with mobility data.

It provides the following services:

* multi-modal journeys computation
* line schedules
* next departures
* exploration of public transport data
* search & autocomplete on places
* and sexy things such as isochrones

Have a look at the [examples below](#some-examples) to learn what services we provide and how to use them.

### Approach

Navitia is an open-source web API, **initially** built to provide traveler information on urban transportation networks.<br>Its main purpose is to provide day-to-day informations to travelers.<br>Over time, Navitia has been able to do way more, _sometimes_ for technical and debuging purpose _or_ because other functional needs fit quite well in what Navitia can do _or_ just because it was quite easy and super cool.

Technically, Navitia is a [HATEOAS](https://en.wikipedia.org/wiki/HATEOAS) API that returns JSON formated results.

### Who's who

Navitia is instanciated and exposed publicly through [api.navitia.io](https://api.navitia.io).<br>Developments on Navitia are lead by Hove (previously Kisio Digital and CanalTP).<br>Hove is a subsidiary of Keolis (itself a subsidiary of SNCF, French national railway company).

### About "sandbox" coverage

<aside class="notice">
Every example is based on fake data. Sandbox data is made up of few fake metro lines from a single network on the area of Paris only. If you want to play with real data, use your token and explore the world!
</aside>

<aside class="notice">
A fake token is used in examples below: be aware that this token is really limited ("sandbox" coverage only).</br>
You have to use yours to get real services on real data (your token has no access to "sandbox" coverage).
</aside>

First step
---------------
> Your token is available on your [navitia.io account page](https://navitia.io/se-connecter/).

> It sounds like "3b036afe-0110-4202-b9ed-99718476c2e0"

[Contact us](https://hove.com/en/solutions/navitia-api-insights/#3) to get a token.

<aside class="notice">
The token obtained will be private, you should avoid sharing it publicly (beware if you share code examples, url or screenshot).</br>
This will avoid having someone use your request quota.
</aside>

Second step
---------------

``` shell
# You can use curl to request Navitia
$ curl 'https://api.navitia.io/v1/'
```
Go to the API <https://api.navitia.io>

The simpliest way is to use a web browser.
Our humble opinion is that [firefox browser](https://www.getfirefox.com) and a json viewer extension like [JSON Lite](https://addons.mozilla.org/fr/firefox/addon/json-lite/) is a good setup.

Third step
---------------

``` shell
# In a curl way, with our fake token
$ curl 'https://api.navitia.io/v1/coverage/sandbox/' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'
```

Use the token: if you use a modern web browser, you only have to **paste it** in the **user name area**,
with **no password**.

![Put token in Firefox browser](/images/firefox_token.png)

Or, in a simplier way, you can add your token in the address bar like:

<aside class="success">
<a href="https://3b036afe-0110-4202-b9ed-99718476c2e0@api.navitia.io/v1/coverage/sandbox/lines">https://3b036afe-0110-4202-b9ed-99718476c2e0@api.navitia.io/v1/coverage/sandbox/lines</a>
</aside>

See [authentication](#authentication) section to find out more details on **how to use your token** when coding.

#### Then,

use the API! The easiest is probably to jump to [Examples](#some-examples) below.

At some point you will want to read [transport public lexicon](#lexicon).

Navitia for humans
------------------

>[Try a basic request on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox&token=3b036afe-0110-4202-b9ed-99718476c2e0)

>[Try a journey request on Navitia playground](https://playground.navitia.io/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fjourneys%3Ffrom%3D2.3749036%253B48.8467927%26to%3D2.2922926%253B48.8583736%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

<aside class="success">
if you only want to challenge Navitia functionality, take your token and go to the <a href="https://playground.navitia.io">navitia playground website </a>.
</aside>

![Try Navitia playgroung](/images/navitia_playground.png)

Wrappers
--------------

To help you in the building of your project, there are some wrappers implemented (by Hove or not) to query the API Navitia:

|Language / Framework |Plugin                                              |
|---------------------|----------------------------------------------------|
|PHP5                 |<https://github.com/hove-io/NavitiaComponent>       |
|Python               |<https://github.com/leonardbinet/navitia_client>    |

<h2 id="about-data">About the data</h2>

The street network is extracted from [OpenStreetMap](https://www.openstreetmap.org). The public transport data are provided by networks that provide their timetables as open data. Some data improvements are achieved by Hove and are published back there <https://data.gouv.fr>.

Getting help
------------

>[Try openAPI / swagger](https://api.navitia.io/v1/schema)

All available functions are documented in [integration part](#interface).

<aside class="success">
    If you want to go further, there is an Swagger-openAPI documentation at https://api.navitia.io/v1/schema
</aside>

In order to report bug and make feature requests please use our github navitia project
<https://github.com/hove-io/navitia/issues>.
