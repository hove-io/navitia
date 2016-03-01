A small introduction to public transportation data
==================================================

Introduction
------------

The world of public transportation is really specific, you will see here a small
introduction to it. A lot of people in the world of public transportation use
different words to speak about the same thing. Here are the definitions of terms
employed in www.navitia.io

Stop Point
----------

A stop point is the physical object were someone waits for his bus
(subway, or whatever type of vehicle you can have in a public transport system).

Stop Area
---------

A stop area is a collection of stop points. Generally there are at least two stop
points per stop area, one per direction of a line. Now think of a hub, you will
have more than one line. Therefore your stop area will contain more than two stop
points. In particular cases your stop area can contain only one stop point.

Some datasets do not contain any stop area.


Connection
----------

This object links two stop points together (named origin and
destination). It is the walkable part of a journey.


Journey Pattern
---------------

A journey pattern is an ordered list of stop points. Two vehicles that serve exactly the
same stop points in exactly the same order belong to to the same journey pattern.


Route
-----

A route is a collection of journey pattern that match the same commercial direction.

<aside class="warning">
    The GTFS specification uses the term <b>route</b> for the object we call <b>line</b>. It does not
    have this notion of commercial direction.
</aside>

Line
----

A line is a collection of routes. Most of the time you'll just have just two routes.
This object is called *route* in the GTFS format.

Stop Time
-----------

A stop time represents the time when a bus is planned to arrive and to leave a
stop point.

