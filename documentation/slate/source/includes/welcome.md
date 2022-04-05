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
> Your token is available on your [navitia.io account page](https://espace-client.navitia.io/).

> It sounds like "3b036afe-0110-4202-b9ed-99718476c2e0"

Get a token here <https://navitia.io/inscription/>. We need your mail to stay in touch when Navitia changes.

<aside class="notice">
The token obtained is private, you should avoid sharing it publicly (beware if you share code examples, url or screenshot).</br>
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

>[Try a basic request on Navitia playground](https://hove-io.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox&token=3b036afe-0110-4202-b9ed-99718476c2e0)

>[Try a journey request on Navitia playground](https://hove-io.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fjourneys%3Ffrom%3D2.3749036%253B48.8467927%26to%3D2.2922926%253B48.8583736%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

<aside class="success">
if you only want to challenge Navitia functionality, take your token and go to the <a href="https://hove-io.github.io/navitia-playground">navitia playground website </a>.
</aside>

![Try Navitia playgroung](/images/navitia_playground.png)

Wrappers
--------------

To help you in the building of your project, there are some wrappers implemented (by Kisio or not) to query the API Navitia:

|Language / Framework |Plugin                                              |
|---------------------|----------------------------------------------------|
|PHP5                 |<https://github.com/CanalTP/NavitiaComponent>       |
|Symfony 2 or 3       |<https://github.com/CanalTP/NavitiaBundle>          |
|Python               |<https://github.com/leonardbinet/navitia_client>    |
|Python               |<https://github.com/CanalTP/navitia_python_wrapper> |

<h2 id="about-data">About the data</h2>

The street network is extracted from [OpenStreetMap](https://www.openstreetmap.org). The public transport data are provided by networks that provide their timetables as open data. Some data improvements are achieved by Kisio Digital and are published back there <https://navitia.opendatasoft.com>.

Want to know if your city is in Navitia? Know if a special contributor is used? You can either search in [datasets](#datasets) of the different [coverages](#coverage). Or use the filter provided on our data catalog <https://navitia.opendatasoft.com>.

<aside class="success">
    We are gradually supporting more and more cities. If your city has open public transport data and is missing,
    check our googlegroup and drop us a note at <a href="mailto:navitia@googlegroups.com">navitia@googlegroups.com</a>.
    </br>
    We will try to add it to navitia.io
</aside>
<aside class="notice">
    Please note that the creation of a new coverage on our servers requires resources,
    so on some data integrations we try to make sure that the amount of requests justifies the use of those resources.
    </br>
    Your feedback and requests are very welcome so we choose the most useful new coverages depending on available resources.
</aside>

Getting help
------------

>[Try openAPI / swagger](https://api.navitia.io/v1/schema)

All available functions are documented in [integration part](#interface).
If you want to go further, there is an Swagger-openAPI documentation at [https://api.navitia.io/v1/schema](https://api.navitia.io/v1/schema)

A mailing list is available to ask questions or request [new data](#about-data) integrations: <a href="mailto:navitia@googlegroups.com">navitia@googlegroups.com</a>

In order to report bug and make feature requests please use our github navitia project
<https://github.com/CanalTP/navitia/issues>.

Stay tuned on twitter [@navitia](https://twitter.com/navitia).

At last, we are present on the network `matrix.org`, channel [`#navitia:matrix.org`](https://riot.im/app/#/room/#navitia:matrix.org).
