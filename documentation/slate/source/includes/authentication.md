Authentication
==============

Authentication is required to use **navitia.io**. When you register we will give you 
an authentication key that must accompany each API call you make.

**Navitia.io** uses [Basic HTTP authentication](http://tools.ietf.org/html/rfc2617#section-2) 
for authentication, where the username is the key, and password remains empty.


For example, in a [Curl](https://en.wikipedia.org/wiki/CURL) way, you can request either :

`curl https://api.navitia.io/v1/coverage -u 01234567-89ab-cdef-0123-456789abcdef:`

(don't forget ``:`` after the key)

or

`curl -H 'Authorization: 01234567-89ab-cdef-0123-456789abcdef' 'https://api.navitia.io/v1/coverage'`

or

`curl https://01234567-89ab-cdef-0123-456789abcdef@api.navitia.io/v1/coverage`

