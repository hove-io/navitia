#Autocomplete

This RFC describes how kraken's autocomplete works.

There are two search modes managed by the *search_type* parameter. The autocompletion starts by doing a request with
`search_type=0`, this uses a prefix search, if no results are found, a second search is done with `search_type=1` to 
use the n-gram.
The *search_type* parameter also changes the sorting/scoring.

## Search
TODO
But most of the time, this isn't the problem.

## Scoring
The "scoring" is done once we have found all our candidates for each requested type.

Every result has four properties used for the "scoring":
  - the *quality* that has been computed during the search, it should represent how much an object is matching
        the query. So an exact match should have a score of 100, while an object matching only one term of a three
        terms' query should have a score of 30. (In reality it's more complicated than that).
  - the *global score*: computed at the "binarisation" is the *importance* of a object, each type has it's own formula:
    - admin: based on the number of *ways* of the admin or the number of *stop_points* associated to it, if it doesn't have any way. This score is normalized with the formula: `score = log(score+2)*10`.
    - stop_area: the number of *stop_points* associated to the stop_area, normalized as a integer between 0 and 100,
      to which is added the score of its first admin of level 8.
    - stop_point: the score is equal to the score of its first admin of level 8.
    - way: the score is equal to the score of its first admin of level 8.
    - poi: the score is equal to the score of its first admin of level 8.
    - All other objects have the same score of 0.
  - the length of the longest common substring between the searched string and the indexed string.
  - the position of this substring in the indexed string.


If *search_type=1*, we start by removing unwanted results: we keep the N results with the best *quality*
where N is the number of result want.

The results are sorted by type, with the following order:
  - network
  - commercial_mode
  - admin
  - stop area
  - poi
  - address
  - line
  - route
  - other (stop_points by example)


Foreach types, we have the follwing order:
In firsts positions, we have the results with a 100 quality (exact match). After them, the order is based
on the *global score*, then by the length of the longest, then by the position of this substring, 
and finally by the quality.


##What does it mean?
If a request returns a lot of admins, the response will be filled with the ten most important admins 
and won't have any stop_area or way.

Most of the times the complain we got on the autocompletion is that a stop_area can't be found.
In fact the stop_area isn't in the ten first results, thus it isn't visible. This is due to how the "scoring" is done, when `search_mode=0` the order of the response is only based on the *global score* (aka. the *importance* of the
object) and not on how much it matchs the request.
