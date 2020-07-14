# Street Networks Services

## Here

Provide a external API that delivers street network solution with realtime traffic informations.<br>
Navitia uses it to perform car sections.<br>
Here documentation : https://developer.here.com/

### How to connect Here with Navitia.

Insert into **Jormun** configuration:

```
 "street_network": [
    {
        "class": "jormungandr.street_network.here.Here",
        "modes": ["car", "car_no_park"],
        "args": {
            "service_base_url": "route.ls.hereapi.com/routing/7.2",
            "apiKey": "Token",
            "timeout": 20,                      # optional
            "realtime_traffic": true,           # optional
            "language": 'english',              # optional
            "matrix_type": "simple_matrix",     # optional
            "max_matrix_points": 100,           # optional
        }
    }
 ]
```

Available optional parameters list:
* timeout: circuit breaker timeout. By default 10 secs
* realtime_traffic: activation of realtime traffic informations - true/false. By default, true
* language: the selected language for guidance instruction - list available bellow. By default, english
    * afrikaans
    * arabic
    * chinese
    * dutch
    * english
    * french
    * german
    * hebrew
    * hindi
    * italian
    * japanese
    * nepali
    * portuguese
    * russian
    * spanish
* matrix_type: the matrix method - simple_matrix/multi_direct_path. By default simple_matrix
* max_matrix_points: the max number of allowed matrix points. By default 100 (the maximum)

### How to debug

You can easily override parameters for tests inside requests.<br>
List of available API parameters:
* _here_realtime_traffic: true/false
* _here_language: "english" or "french" or "dutch" ...
* _here_matrix_type: simple_matrix/multi_direct_path
* _here_max_matrix_points: int value [1-100]

Example:

```
http://navitia.io/v1/coverage/coverage_name/journeys?from=2.13376%3B48.86333&to=2.33443%3B48.84189&first_section_mode%5B%5D=car&_here_realtime_traffic=true&_here_max_matrix_points=50&_here_language=french
```
