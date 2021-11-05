Lexicon
=======

<h2 id="lexicon-introduction">Introduction</h2>

The world of public transportation is really specific, you will see here a small
introduction to it. A lot of people in the world of public transportation use
different words to speak about the same thing. Here are the definitions of terms
employed in www.navitia.io

You can read a mobility lexicon at
<https://github.com/OpenTransport/vocabulary/blob/master/vocabulary.md>

You will often read "PT" in this document. PT stands for public transport.

<h2 id="lexicon-stop-point">Stop Point</h2>

A stop point is the physical object were someone waits for his bus
(subway, or whatever type of vehicle you can have in a public transport system).

<h2 id="lexicon-stop-area">Stop Area</h2>

A stop area is a collection of stop points. Generally there are at least two stop
points per stop area, one per direction of a line. Now think of a hub, you will
have more than one line. Therefore your stop area will contain more than two stop
points. In particular cases your stop area can contain only one stop point.

Some datasets do not contain any stop area.

<h2 id="lexicon-connection">Connection</h2>

This object links two stop points together (named origin and destination).
It is the walkable part in a transfer between 2 public transport vehicles.

<h2 id="lexicon-journey-pattern">Journey Pattern</h2>

A journey pattern is an ordered list of stop points. Two vehicles that serve exactly the
same stop points in exactly the same order belong to to the same journey pattern.

<h2 id="lexicon-route">Route</h2>

A route is a collection of journey pattern that match the same commercial direction.

<aside class="warning">
    The GTFS specification uses the term <b>route</b> for the object we call <b>line</b>. It does not
    have this notion of commercial direction.
</aside>

<h2 id="lexicon-line">Line</h2>

A line is a collection of routes. Most of the time you'll just have just two routes.
This object is called *route* in the GTFS format.

<h2 id="lexicon-stop-time">Stop Time</h2>

A stop time represents the time when a bus is planned to arrive and to leave a
stop point.
