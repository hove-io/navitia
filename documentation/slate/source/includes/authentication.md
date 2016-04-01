Authentication
==============

You must authenticate to use **navitia.io**. When you register we give
you an authentication key to the API.

You must use the [Basic HTTP authentication](http://tools.ietf.org/html/rfc2617#section-2), 
where the username is the key, and without password.

For example, in a [Curl](https://en.wikipedia.org/wiki/CURL) way, you can request either :

`curl https://api.navitia.io/v1/coverage -u 01234567-89ab-cdef-0123-456789abcdef:`

(don't forget ``:`` after the key)

or

`curl -H 'Authorization: 01234567-89ab-cdef-0123-456789abcdef' 'https://api.navitia.io/v1/coverage'`

or

`curl https://01234567-89ab-cdef-0123-456789abcdef@api.navitia.io/v1/coverage`

