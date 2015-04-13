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
reloader_at: ./manage_tyr.py at_reloader
```

.env:
```
PYTHONPATH=.:../navitiacommon
TYR_CONFIG_FILE=dev_settings.py
```
with dev_settings.py the tyr settings file

With those 2 files, running the commande ```honcho start``` in the tyr directory will launch all tyr services.


## Data integration

TODO

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
        "id": 1,
        "is_free": false,
        "name": "picardie"
    },
    {
        "id": 2,
        "is_free": false,
        "name": "PaysDeLaLoire"
    }
]
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
        "login": "foo"
    },
    {
        "email": "alex@example.com",
        "id": 3,
        "login": "alex"
    }
]
```

Look for a user with his email:

    GET $HOST/v0/users/?email=foo@example.com

```json
[
    {
        "email": "foo@example.com",
        "id": 1,
        "login": "foo"
    }
]
```

Get all a user information:

    GET $HOST/v0/users/$USERID/

```json
{
    "authorizations": [],
    "email": "alex@example.com",
    "id": 3,
    "keys": [],
    "login": "alex"
}
```


To create a user, parameters need to given in the query's string or in json send in the body

Note: email addresses are validated via api not only on the format but on it's existence. However no email are send.

#### Parameters

name   | description                                                                         | required | default                |
-------|-------------------------------------------------------------------------------------|----------|------------------------|
email  | adress email of the user                                                            | true     |                        |
login  | login of the user                                                                   | true     |                        |
type   | type of the user between: [with_free_instances, without_free_instances, super_user] | false    | with_free_instances    |


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
