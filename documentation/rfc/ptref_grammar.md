# Public Transit Referential grammar

It's possible to do advanced query to the public transit referential. The query is in the form `/v1/coverage/id/collections?filter=query`. This is the documentation of the `query` syntax. For simplicity, the queries in this documentation will not be urlencoded, but it must be in practice.

Each query is composed of 2 parts: the collection to be asked (`collections` in the example), and the query itself that describe the wanted objects.

The valid collections are `commercial_modes`, `companies`, `connections`, `contributors`, `datasets`, `disruptions`, `journey_patterns`, `journey_pattern_points`, `lines`, `networks`, `physical_modes`, `poi_types`, `pois`, `routes`, `stop_areas`, `stop_points` and `vehicle_journeys`.

The `traffic_reports` endpoint builds its response on the `networks` collection.

The `line_reports` endpoint builds its response on the `lines` collection.

The `route_schedules` endpoint builds its response on the `routes` collections.

The `arrivals`, `departures` and `stop_schedules` endpoint builds their responses on the `journey_pattern_points` collection.

## Predicates

The elementary building block of a query is the predicate. They define a set of requested objects.

### Elementary predicates

There is 2 elementary predicates: `all` and `empty`. As expected, `all` returns all the queried objects while `empty` returns none.

Examples:
 * `/v1/coverage/id/stop_areas?filter=all`: returns all the stop areas of the coverage.
 * `/v1/coverage/id/stop_areas?filter=none`: returns 0 stop areas (you get a 404).
 
### Method predicates

Method predicates are in the form `collection.method(string, string, ...)` where:
 * `collection` is a collection name (as the collections queried, but singular);
 * `method` is a method name, corresponding to the filter on the `collection`;
 * arguments to the method between `(` and `)`, as strings, separated by `,`.
 
#### Strings
 
A string can be:
 * an escaped string, begining and ending with `"`, with `\x` transformed as `x` within the string
 * a basic string, composed of any letter, digit, `_`, `.`, `:`, `;`, `|` and `-`.
 
Basic strings are great to easily type numbers and navitia coordinates. It can also handle most of the identifiers, but be warned that you will run into troubles if you use the basic string syntax in a programatic way (think of identifiers with spaces or accentuated letters).

For programatic usage, it is recommended to:
 * substitute `"` by `\"` and `\` by `\\`
 * surround the string with `"`

examples:
 * `1337`
 * `-42`
 * `-123.45e-12`
 * `2.37715;48.846781`
 * `"I can have spaces and accentuated letters: éèà"` (decoded as `2.37715;48.846781`)
 * `"a \" and a \\ within a string"` (decoded as `a " and a \ within a string`)

#### Availlable methods

In the following table, if the invocation is begining with `collection`, any collection can be used, else the method will only work for the specified collection.

TODO

#### Interpretation

### Field test predicates

### DWITHIN predicate

## Expressions

## Sub request

## Some examples

