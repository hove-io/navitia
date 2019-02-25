# Documentation of the routing algorithm

## The implemented algorithm

### Abstract

a quick view

### RAPTOR and the specificities

The routing algorithm is build around RAPTOR, a routing algorithm developped at Microsoft and published in 2012. The paper can be found [here](https://www.microsoft.com/en-us/research/wp-content/uploads/2012/01/raptor_alenex.pdf).

Basically, you give to RAPTOR the set of journey patterns it can use, the set of stop points it can use (for transfers), the datetime you can reach the stop points and the direction. It gives you, for each stop points and each number of transfers, the earliest arrival datetime.

* datastructures
* stay in
* ITL
* Something else?

### First pass

* needed? merged with previous?

### Second pass

* goal
* selecting the second pass launches (max_extra_second_pass)
* bounding the extra second passes
* reusing the bound of the first passes to limit the search space

### Solution reader

* goal
* the objectives
* the search space
* the branching scheme

### Advantages and drawbacks

advantages:
* diversity
* performance
* flexibility

drawbacks:
* not exact walking objective
* difficulty to add objectives
* RAPTOR variants quite complex and not really combinable

## Some ideas for improvement

* RAPTOR, CSA and others
* objectives
