#Autocomplete

This RFC describes how the kraken autocomplete is working.

There is two search mode managed by the *search_type* parameter, the autocompletion start by doing a request with
search_type=0, this use a prefix search, if no results are found a second search is done with search_type=1 to 
use the n-gram.
The search_type parameter also change the sorting/scoring.

## Search
TODO
But most of the time this isn't the problem

## Scoring
The "scoring" is done once we have found all our candidates for each requested types.

Every result has four properties used for the "scoring"
  - the *quality* that has been computed during the search, it should represent how much an object is matching
        the query, so an exact match should have a score of 100 while a object matching only one term of a three
        term query should have a score of 30. (In reality it's more complicated than that)
  - the *global score*: computed at the "binarisation" is the *importance* of a object, each type has it's own formula:
    - admin: based on the number of *way* of the admin or the number of *stop_points* associated to the admin if it doesn't have any way. This score is normalized with the formula: `score = log(score+2)*10`
    - stop_area: the number of stop_point associated to the stop_area normalized as a integer between 0 and 100,
      on with is added the score of its first admin of level 8.
    - stop_point: the score is equal to the score of its first admin of level 8
    - way: the score is equal to the score of its first admin of level 8
    - poi: the score is equal to the score of its first admin of level 8
    - Other objects all have the same score of 0
  - the length of the longest common substring between the string to search and the indexed string
  - the position of this substring in the indexed string


If *search_type=1* we start by removing unwanted results: we keep the N results with the best *quality*
where N is the number of result wanted

The results are sorted by types, the order is:
  - network
  - commercial_mode
  - admin
  - stop area
  - poi
  - address
  - line
  - route
  - other (stop_points by example)


Inside each type the order is the following:
In firsts positions we get the results with a quality of 100 (exact match), after them the order is based
on the *global score*, then by the length of the longest, then by the position of this substring, 
and finally by the quality.


##What does it mean?
If a request return a lot of admin the response will be filled with the ten most important admins 
and won't have any stop_area or way.

Most of the times the complain we got on the autocompletion is that a stop_area can't be found.
In fact the stop_area isn't in the ten first results so it isn't visible. This is due to how the "scoring" is done, when search_mode=0 the order of the response is only based on the *global score* aka the *importance* of the
object and not on how much it match the request.
