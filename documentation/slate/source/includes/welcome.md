Getting started
=====================

Overview
--------

*navitia.io* is the open API for building cool stuff with mobility data.
It provides the following services

* journeys computation
* line schedules
* next departures
* exploration of public transport data / search places
* and sexy things such as isochrones

navitia is a [HATEOAS](http://en.wikipedia.org/wiki/HATEOAS) API that returns JSON formated results, available at <https://api.navitia.io>

Have a look at the examples below to learn what services we provide and how to use them. 


<aside class="notice">
A fake token is used in examples below: be aware that this token is really limited. You should use yours to get better services on real data.
</aside>

First step
---------------
``` shell
#your token is in your confirmation mail. It sounds like "3b036afe-0110-4202-b9ed-99718476c2e0"
```

Get a token here <http://navitia.io/register/>. We need your mail to stay in touch when Navitia changes.

Second step
---------------
``` shell
#you can use curl to request Navitia
$ curl 'https://api.navitia.io/v1/'
```

Go to the API  <https://api.navitia.io>

The simpliest way is to use a web browser. 
Our humble opinion is that [firefox browser](http://www.getfirefox.com) and a json viewer extension like [JSONView](https://addons.mozilla.org/fr/firefox/addon/jsonview/) is a good setup.

Third step
---------------
``` shell
#in a curl way, with our fake token
$ curl 'https://api.navitia.io/v1/coverage/sandbox/' -H 'Authorization: 3b036afe-0110-4202-b9ed-99718476c2e0'
```

Use the token : if you use a modern web browser, you only have to **paste it** in the **user name area**,
with **no password**.

![Put token in Firefox browser](/images/firefox_token.png)

Or, in a simplier way, you can add your token in the address bar like :

<aside class="success">
<a href="https://3b036afe-0110-4202-b9ed-99718476c2e0@api.navitia.io/v1/coverage/sandbox/lines">https://3b036afe-0110-4202-b9ed-99718476c2e0@api.navitia.io/v1/coverage/sandbox/lines</a>
</aside>

See [authentication](#authentication) section to find out more details on **how to use your token** when coding.

#### Then,
use the API ! The easiest is probably to jump to [Examples](#some_examples) below.

At some point you will want to read:

- [transport public lexicon](#lexicon)
- [Ressources overview](#ressources)

I'm just a human
----------------

OK, if you only want to challenge Navitia functionality, take your token and go to the [navitia playground website](#http://canaltp.github.io/navitia-playground).

For example, you can easy request for a journey there:

[http://canaltp.github.io/navitia-playground/](#http://canaltp.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fcoverage%2Fsandbox%2Fjourneys%3Ffrom%3D2.3749036%253B48.8467927%26to%3D2.2922926%253B48.8583736%26&token=3b036afe-0110-4202-b9ed-99718476c2e0)

About the data
--------------

The street network is extracted from [OpenStreetMap](http://www.openstreetmap.org). The public transport data are provided by networks that provide their timetables as open data. Some data improvements are achieved by Kisio Digital and are published back there <https://navitia.opendatasoft.com>

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

