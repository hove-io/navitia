# Ridesharing Services

## Blablacar

Provides an external API that delivers ridesharing offers.<br>
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


## Klaxit

Also provides an external API that delivers ridesharing offers.<br>
As with other connectors Navitia exposes the offers inside **ridesharing_journeys** json output field.<br>
Klaxit documentation : https://dev.klaxit.com/swagger-ui?url=https://via.klaxit.com/v1/swagger.json

### How to connect Klaxit with Navitia.

Insert into **Jormun** configuration:

```
 "ridesharing": [
    {
        "class": "jormungandr.scenarios.ridesharing.klaxit.Klaxit",
        "args": {
            "service_url": "https://via.klaxit.com/v1/carpoolJourneys",
            "api_key": "Token",
            "network": "Klaxit",
            "timeout": 10,              # optional (defaults to 2 in secs)
            "timedelta": 3600,          # optional (defaults to 3600 in secs)
            "departure_radius": 3,      # optional (defaults to 2 in km)
		    "arrival_radius": 3         # optional (defaults to 2 in km)
        }
    }
 ]
```

## Karos

Also provides an external API that delivers ridesharing offers.<br>
As with other connectors Navitia exposes the offers inside **ridesharing_journeys** json output field.<br>
Karos documentation : https://app.swaggerhub.com/apis/Karos/karos-api/1.1.0-oas3

### How to connect Karos with Navitia.

Insert into **Jormun** configuration:

```
 "ridesharing": [
    {
        "class": "jormungandr.scenarios.ridesharing.karos.Karos",
        "args": {
            "service_url": "https://ext.karos.fr:443/carpoolJourneys",
            "api_key": "Token",
            "network": "Karos",
            "timeout": 10,              # optional (defaults to 2 in secs)
            "timedelta": 3600,          # optional (defaults to 3600 in secs)
            "departure_radius": 3,      # optional (defaults to 2 in km)
		    "arrival_radius": 3         # optional (defaults to 2 in km)
        }
    }
 ]
```
