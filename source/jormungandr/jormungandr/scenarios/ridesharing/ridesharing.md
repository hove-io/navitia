# Ridesharing Services

## Blablacar

Provide a external API that delivers ridesharing offers.<br>
Navitia exposes the offers inside **ridesharing_journeys** json output field.<br>
Blablacar documentation : https://www.blablalines.com/public-api-v2#search-get

### How to connect Blablacar with Navitia.

Insert into **Jormun** configuration:

```
 "ridesharing": [
    {
        "class": "jormungandr.scenarios.ridesharing.blablacar.Blablacar",
        "args": {
            "service_url": "https://api.blablalines.com/2/third_party/public/search",
            "api_key": "Token",
            "network": "Blabalcar",
            "timeout": 10,            # optional
            "timedelta": 3600,        # optional
        }
    }
 ]
```

Available optional parameters list:
* timeout: circuit breaker timeout. By default 10 secs
* timedelta: The timedelta around passenger departure’s time (in seconds, defaults to 3600)

