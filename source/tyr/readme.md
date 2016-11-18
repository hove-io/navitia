# Tyr

This module is the conductor of Navitia.

Note: the installation of this module is not mandatory to have a fully operational Navitia.

In a running environment, Tyr is responsible for:

 * detecting new data and integrating them in the system
 * handling different kraken instances and their functional configuration
 * the authentication

Tyr uses the same database as Jormungandr


## Setup

### Overview

Tyr is based on flask, so it is used like:

    TYR_CONFIG_FILE=<your_config_file.py> manage_tyr.py <command>

Note: tyr depends on navitiacommon, so the navitiacommon package needs to be found.

There are multiple ways to make it available but one easy way is to change the PYTHONPATH variable:

    PYTHONPATH=<path_to_tyr>:<path_to_navitiacommon> TYR_CONFIG_FILE=<your_config_file.py> manage_tyr.py <command>

To list all available command, call manage_py with the -h option.

To setup the tyr database you need to call:

    PYTHONPATH=<path_to_tyr>:<path_to_navitiacommon> TYR_CONFIG_FILE=<your_config_file.py> manage_tyr.py db upgrade


### Services

Tyr is split in 3 different services:

* tyr beat
    Scheduler based on celery. Launches a worker when a task need to be done

* tyr worker
    Launched by tyr beat. They are the one getting the job done

    If no worker is launched, a lot of tyr command will not work

* tyr reloader
    Handle the reloading of the kraken (this is the old way of handling real time data)


### Example with honcho

In CanalTP production environments tyr is installed as a service via debian packages, but to configure it on dev environment honcho can be used.

Honcho is handy to launch all the different services.

install honcho:

    pip install honcho

In the tyr directory create 2 files:

Procfile:
```
web: ./manage_tyr.py runserver
worker: celery worker -A tyr.tasks
scheduler: celery beat -A tyr.tasks
```

.env:
```
PYTHONPATH=.:../navitiacommon
TYR_CONFIG_FILE=dev_settings.py
```
with dev_settings.py the tyr settings file

With those 2 files, running the commande ```honcho start``` in the tyr directory will launch all tyr services.

##Running tests

You need to install the dependancies for developements:
```
pip install -r requirements_dev.txt
```
You will need docker on your machine, it will be used for spawning a database.

You will also need the python protobuf files from Navitia, so you should use CMake target `protobuf_files` for that :
```
cd <path/to/build/directory>
cmake <path/to/navitia/repo>/source/
make protobuf_files
```

then run the test:
```
PYTHONPATH=../navitiacommon py.test tests
```
Or if you use honcho:
```
honcho run py.test tests
```


## Data integration

You can get the last data integrations using the jobs endpoint

    GET $HOST/v0/jobs/

You may want to specify an instance to filter these jobs :

    GET $HOST/v0/jobs/<INSTANCE>

A job is created when a new dataset is detected by tyr_beat.
You can also trigger a data integration by posting your dataset to the job endpoint filtered by instance.

## Authentication

With the authentication we can associate a user account to

* a kraken instance
* the api consumption
* some statistics

For the authentication webservice to work, tyr_beat and tyr_worker need to be run.

### Subcription

Subscription is done via calls to Tyr webservice. Tyr handles:

* Instances
* Apis
* Users
* Keys
* Authorization

The api returns 4XX response on failure, with a json message containing the attribute error and the message.

For a simple user account creation see the [example section](#simple-example)

### Tyr API

#### Instances

Returns the list of instances

    GET $HOST/v0/instances/

response:
```json
[
        {
            "min_tc_with_bss" : 300,
            "journey_order" : "arrival_time",
            "max_duration" : 86400,
            "max_bss_duration_to_pt" : 1800,
            "max_nb_transfers" : 10,
            "bike_speed" : 4.1,
            "walking_transfer_penalty" : 120,
            "night_bus_filter_base_factor" : 3600,
            "walking_speed" : 1.12,
            "id" : 1,
            "max_duration_fallback_mode" : "walking",
            "priority" : 0,
            "car_speed" : 11.11,
            "min_tc_with_car" : 300,
            "min_tc_with_bike" : 300,
            "min_bike" : 240,
            "max_walking_duration_to_pt" : 1800,
            "is_free" : false,
            "min_car" : 300,
            "max_bike_duration_to_pt" : 1800,
            "max_duration_criteria" : "time",
            "bss_provider" : true,
            "name" : "jdr",
            "scenario" : "new_default",
            "bss_speed" : 4.1,
            "discarded" : false,
            "min_bss" : 420,
            "night_bus_filter_max_factor" : 3,
            "max_car_duration_to_pt" : 1800
        },
        {
            "min_tc_with_bss" : 300,
            "journey_order" : "arrival_time",
            "max_duration" : 86400,
            "max_bss_duration_to_pt" : 1800,
            "max_nb_transfers" : 10,
            "bike_speed" : 4.1,
            "walking_transfer_penalty" : 120,
            "night_bus_filter_base_factor" : 900,
            "walking_speed" : 1.12,
            "id" : 3,
            "max_duration_fallback_mode" : "walking",
            "priority" : 0,
            "car_speed" : 11.11,
            "min_tc_with_car" : 300,
            "min_tc_with_bike" : 300,
            "min_bike" : 240,
            "max_walking_duration_to_pt" : 1800,
            "is_free" : true,
            "min_car" : 300,
            "max_bike_duration_to_pt" : 1800,
            "max_duration_criteria" : "time",
            "bss_provider" : true,
            "name" : "fr-idf",
            "scenario" : "new_default",
            "bss_speed" : 3.5,
            "discarded" : false,
            "min_bss" : 240,
            "night_bus_filter_max_factor" : 1.5,
            "max_car_duration_to_pt" : 1800
        }
]
```

You can also update the config of the instance with a PUT request.

For instance, if you have many instances on the same geographical area, you may want to set the priority property to define which one should be used first when requesting navitia APIs without coverage ($navitia_uri/v1/journeys, $navitia_uri/v1/coord/, etc)
```bash
    curl 'http://localhost:5000/v0/instances/<INSTANCE>?priority=1' -X PUT
```

#### Api

Returns the list of available API. For the moment there is only one, the "ALL" API.

    GET $HOST/v0/api/

```json
[
    {
        "id": 1,
        "name": "ALL"
    }
]
```

#### Users

Delete, add or update a user

To get the list of users

    GET $HOST/v0/users/

```json
[
    {
        "email": "foo@example.com",
        "id": 1,
        "login": "foo",
        "type": "with_free_instances",
        "has_shape": false,
        "shape": null,
        "billing_plan": {
            "max_request_count": 3000,
            "name": "foo",
            "default": true,
            "max_object_count": 60000,
            "id": 1
        },
        "end_point": {
            "default": false,
            "hostnames": [],
            "id": 2,
            "name": "foo"
        }
    },
    {
        "email": "alex@example.com",
        "id": 3,
        "login": "alex",
        "type": "with_free_instances",
        "has_shape": true,
        "shape": {},
        "billing_plan": {
            "max_request_count": 3000,
            "name": "foo",
            "default": true,
            "max_object_count": 60000,
            "id": 1
        },
        "end_point": {
            "default": false,
            "hostnames": [],
            "id": 1,
            "name": "alex"
        }
    }
]
```

Look for a user with his email:

    GET $HOST/v0/users/?email=foo@example.com

```json
[
    {
        "billing_plan": {
            "max_request_count": 3000,
            "name": "foo",
            "default": true,
            "max_object_count": 60000,
            "id": 1
        },
        "end_point": {
            "default": false,
            "hostnames": [],
            "id": 1,
            "name": "foo"
        },
        "type": "with_free_instances",
        "email": "foo@example.com",
        "has_shape": false,
        "shape": null,
        "id": 1,
        "login": "foo"
    }
]
```

Get all users for a specific endpoint

    GET $HOST/v0/users/?end_point_id=4

```json
[
    {
        "billing_plan": {
            "max_request_count": 3000,
            "name": "foo",
            "default": true,
            "max_object_count": 60000,
            "id": 1
        },
        "end_point": {
            "default": false,
            "hostnames": [],
            "id": 1,
            "name": "foo"
        },
        "type": "with_free_instances",
        "email": "foo@example.com",
        "has_shape": false,
        "shape": null,
        "id": 1,
        "login": "foo"
    }
]
```

Get all a user information:

    GET $HOST/v0/users/$USERID/

```json
{
    "billing_plan": {
        "max_request_count": 3000,
        "name": "foo",
        "default": true,
        "max_object_count": 60000,
        "id": 1
    },
    "authorizations": [],
    "email": "alex@example.com",
        "has_shape": false,
        "shape": null,
    "id": 3,
    "keys": [],
    "login": "alex",
    "end_point": {
        "default": false,
        "hostnames": [],
        "id": 1,
        "name": "foo"
    },
}
```

#### GET Parameters

name             | description                                                                          | required | default        |
-----------------|--------------------------------------------------------------------------------------|----------|----------------|
disable_geojson  | if false the shape will be fully displayed (otherwise displaying: `null` or `{}`)    | nope     | true          |


Example for `disable_geojson=false` (see default above for `true`)

    GET $HOST/v0/users?disable_geojson=false

```json
[
    {
        "email": "foo@example.com",
        "id": 1,
        "login": "foo",
        "type": "with_free_instances",
        "has_shape": false,
        "shape": null,
        "billing_plan": {
            "max_request_count": 3000,
            "name": "foo",
            "default": true,
            "max_object_count": 60000,
            "id": 1
        },
        "end_point": {
            "default": false,
            "hostnames": [],
            "id": 2,
            "name": "foo"
        }
    },
    {
        "email": "alex@example.com",
        "id": 3,
        "login": "alex",
        "type": "with_free_instances",
        "has_shape": true,
        "shape": {
            "type": "Feature",
            "geometry": {
                "type": "Polygon",
                "coordinates": [ [ [100, 0], [101, 0], [101, 1], [100, 1], [100, 0] ] ]
            },
            "properties": {
                "prop0": "value0",
                "prop1": {
                    "this": "that"
                }
            }
        },
        "billing_plan": {
            "max_request_count": 3000,
            "name": "foo",
            "default": true,
            "max_object_count": 60000,
            "id": 1
        },
        "end_point": {
            "default": false,
            "hostnames": [],
            "id": 1,
            "name": "alex"
        }
    }
]
```




To create a user, parameters need to given in the query's string or in json send in the body

Note: email addresses are validated via api not only on the format but on it's existence. However no email are send.

#### POST Parameters

name         | description                                                                          | required | default                            |
-------------|--------------------------------------------------------------------------------------|----------|------------------------------------|
email        | adress email of the user                                                             | yep      |                                    |
login        | login of the user                                                                    | yep      |                                    |
type         | type of the user between: [with_free_instances, without_free_instances, super_user]  | nope     | with_free_instances                |
end_point_id | the id of the endpoint for this user (in most case the default value is good enough  | nope     | the default end_point (navitia.io) |
shape        | the shape (geojson) to be used for autocomplete for this user                        | nope     | null                               |


    POST /v0/users/?email=alex@example.com&login=alex

Or

    curl -XPOST 'http://127.0.0.1:9000/v0/users/' -d'{"email": "foo@foo.com", "login": "foo"}' -H'content-type: application/json'

If you use the json format as input, boolean need to be passed as string.

```json
{
    "authorizations": [],
    "email": "alex@example.com",
    "id": 3,
    "keys": [],
    "login": "alex"
}
```

#### PUT Parameters

They are the same as POST.
When a parameter is missing, it is not changed.
When PUTing the exact result of a GET (using disable_geojson or not), nothing is changed.

##### Shape modification
`has_shape` is not a parameter (no effect whatsoever).
To modify a shape, PUT the new shape (so far, simple feature only):
```json
{
  "shape": {"type": "Feature",
      "geometry": {
          "type": "Polygon",
          "coordinates": [
              [[100.0, 0.0], [101.0, 0.0], [101.0, 1.0],
               [100.0, 1.0], [100.0, 0.0]]
          ]
      },
      "properties": {
          "prop0": "value0",
          "prop1": {"this": "that"}
      }
  }
}
```
To remove a shape, PUT `{"shape":null}`.


#### Keys

The API handle the user's token

To add a token, call:

    POST /v0/users/$USERID/keys?app_name=foo

```json
{
    "authorizations": [],
    "email": "alex@example.com",
    "id": 3,
    "keys": [
        {
            "id": 1,
            "token": "05fd3dd6-3b27-4470-ad85-60d80cdce487",
            "valid_until": null,
            "app_name": "foo"
        }
    ],
    "login": "alex"
}
```

The token is returned by the API. This token will be need for jormungandr authentication

To delete a token:

    DELETE /v0/users/$USERID/keys/$KEYID

```json
{
    "authorizations": [],
    "email": "alex@example.com",
    "id": 3,
    "keys": [],
    "login": "alex"
}
```

#### Authorizations

This API handle access policy for a given user and a given kraken instance.

This is useful only is the instance is not "free"

    POST /v0/users/$USERID/authorizations/?api_id=$APIID&instance_id=$INSTANCEID

```json
{
    "authorizations": [
        {
            "api": {
                "id": 1,
                "name": "ALL"
            },
            "instance": {
                "id": 1,
                "is_free": false,
                "name": "picardie"
            }
        }
    ],
    "email": "alex@example.com",
    "id": 3,
    "keys": [],
    "login": "alex"
}
```

### Simple example

Here a quick example of how to create a user with access to public instances using curl (but you can use whatever you want):

Note: by default tyr is accessed on localhost:5000

Create a user:

    curl -X POST "localhost:5000/v0/users/?email=toto@canaltp.fr&login=bob"

Note: the email must exists

If the creation went well, a json is returned with an id:

```json
[
    {
        "email": "toto@canaltp.fr",
        "id": 1,
        "login": "bob"
    }
]

```

Use the id in the next query

    curl -X POST "localhost:5000/v0/users/1/keys"

```json
{
    "authorizations": [],
    "email": "toto@canaltp.fr",
    "id": 1,
    "keys": [
        {
            "id": 1,
            "token": "faa80b5f-f747-45bf-ad89-b3f2b29dd4aa",
            "valid_until": null
        }
    ],
    "login": "bob"
}

```


#### EndPoints

Endpoints are used for handling multiple user base with the same plateform. Each user is associated with only one
endpoint. it's possible for a user to have an account in two separate endpoint with the same email used.

you can see all endpoints with the ```end_points``` ressource:

    GET /v0/end_points/

```json
[
    {
        "default": false,
        "hostnames": [
            "bar"
        ],
        "id": 10,
        "name": "foo1"
    },
    {
        "default": true,
        "hostnames": [],
        "id": 1,
        "name": "navitia.io"
    }
]
```

For creating a new one the POST verb must be use and the request must contain a json of this form:

```json
{
    "name": "foo",
    "hostnames": ["foo.com"]
}
```

The hostnames are facultavies, it's only have to be set if we want to enforce the host used for accessing the API.
Update can be done with the PUT verb with the same kind of json.

### Simple example

Here a quick example of how to create a user with access to public instances using curl (but you can use whatever you want):

Note: by default tyr is accessed on localhost:5000

Create a user:

    curl -X POST "localhost:5000/v0/users/?email=toto@canaltp.fr&login=bob"

Note: the email must exists

If the creation went well, a json is returned with an id:

```json
[
    {
        "email": "toto@canaltp.fr",
        "id": 1,
        "login": "bob"
    }
]

```

Use the id in the next query

    curl -X POST "localhost:5000/v0/users/1/keys"

```json
{
    "authorizations": [],
    "email": "toto@canaltp.fr",
    "id": 1,
    "keys": [
        {
            "id": 1,
            "token": "faa80b5f-f747-45bf-ad89-b3f2b29dd4aa",
            "valid_until": null
        }
    ],
    "login": "bob"
}

```

#### PoiTypes
It is possible to define what type of POI should be extracted from OSM for a specific instance.

A type of poi is characterised by an uri and a name.

If nothing if configured, default POI are extracted. At this time the list is as follow:
- uri: "amenity:college" name: "école"
- uri: "amenity:university" name: "université"
- uri: "amenity:theatre" name: "théâtre"
- uri: "amenity:hospital" name: "hôpital"
- uri: "amenity:post_office" name: "bureau de poste"
- uri: "amenity:bicycle_rental" name: "station vls"
- uri: "amenity:bicycle_parking" name: "Parking vélo"
- uri: "amenity:parking" name: "Parking"
- uri: "amenity:police" name: "Police, Gendarmerie"
- uri: "amenity:townhall" name: "Mairie"
- uri: "leisure:garden" name: "Jardin"
- uri: "leisure:park" name: "Zone Parc. Zone verte ouverte, pour déambuler. habituellement municipale"

Let's say you want to see all poi types for the instance fr-bre, you have to call the PoiTypes endpoint for this
instance:

    curl "http://localhost:5000/v0/instances/fr-bre/poi_types"
```json
{
  "poi_types": [
    {"id": "amenity:college", "name": "École"},
    {"id": "amenity:university", "name": "Université"},
    {"id": "amenity:theatre", "name": "Théâtre"},
    {"id": "amenity:hospital", "name": "Hôpital"},
    {"id": "amenity:post_office", "name": "Bureau de poste"},
    {"id": "amenity:bicycle_rental", "name": "Station VLS"},
    {"id": "amenity:bicycle_parking", "name": "Parking vélo"},
    {"id": "amenity:parking", "name": "Parking"},
    {"id": "amenity:police", "name": "Police, gendarmerie"},
    {"id": "amenity:townhall", "name": "Mairie"},
    {"id": "leisure:garden", "name": "Jardin"},
    {"id": "leisure:park", "name": "Parc, espace vert"}
  ],
  "rules": [
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "college"}
      ],
      "poi_type_id": "amenity:college"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "university"}
      ],
      "poi_type_id": "amenity:university"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "theatre"}
      ],
      "poi_type_id": "amenity:theatre"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "hospital"}
      ],
      "poi_type_id": "amenity:hospital"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "post_office"}
      ],
      "poi_type_id": "amenity:post_office"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "bicycle_rental"}
      ],
      "poi_type_id": "amenity:bicycle_rental"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "bicycle_parking"}
      ],
      "poi_type_id": "amenity:bicycle_parking"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "parking"}
      ],
      "poi_type_id": "amenity:parking"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "police"}
      ],
      "poi_type_id": "amenity:police"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "townhall"}
      ],
      "poi_type_id": "amenity:townhall"
    },
    {
      "osm_tags_filters": [
        {"key": "leisure", "value": "garden"}
      ],
      "poi_type_id": "leisure:garden"
    },
    {
      "osm_tags_filters": [
        {"key": "leisure", "value": "park"}
      ],
      "poi_type_id": "leisure:park"
    }
  ]
}
```

If you want to add poi types for an instance you have to POST again all the json (previous AND new poi types):
    
    curl 'http://localhost:5000/v0/instances/fr-bre/poi_types' -X POST -H 'content-type: application/json' -d '{"poi_types": [{"id": "pdv", "name": "Point de vente"},],"rules": [{"osm_tags_filters": [{"key": "amenity:park", "value": "yes"}, {"key": "amenity", "value": "shop"}], "poi_type_id": "pdv"}]}'

For updating a poi_type (only the name can be changed) you have to do the same thing with a PUT.

Finally if you want to delete a poi_type you just have to use the DELETE action:

    curl 'http://localhost:5000/v0/instances/fr-bre/poi_types' -X DELETE

