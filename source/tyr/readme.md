# Tyr

This module is the conductor of Navitia.

Note: the installation of this module is not mandatory to have a fully operational Navitia.



In a running environment, Tyr is responsible for:
 
 * detecting new data and integrating them in the system
 * handling different kraken instances and their functional configuration
 * the authentication


Tyr uses the same database as Jormungandr


## Data integration

TODO

## Authentication

With the authentication we can associate a user account to

* a kraken instance
* the api consumption
* some statistics


### Inscription

Inscription is done via calls to Tyr webservice. Tyr handle:

* Instances
* Apis
* Users
* Keys
* Authorization

 
To create a user with access to public instances, one need only to call:

    POST /v0/users/
    POST /v0/users/userid/keys/

 
The api returns 4XX response on failure, with a json message containing the attribute error and the message.

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

Get all a user information

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
 

To create a user, parameters need to given in the query's body 

Note: email addresses are validated via api not only on the format but on it's existence. Hosever no email are send.

POST /v0/users/?email=alex@example.com&login=alex  

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

POST /v0/users/$USERID/keys  

```json
{
    "authorizations": [],
    "email": "alex@example.com",
    "id": 3,
    "keys": [
        {
            "id": 1,
            "token": "05fd3dd6-3b27-4470-ad85-60d80cdce487",
            "valid_until": null
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

This is usefull only is the instance is not "free"

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

