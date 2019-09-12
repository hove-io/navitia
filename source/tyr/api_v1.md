# Tyr API v1

Tyr API is getting an upgrade.

In order to give homogeneity to every endpoints responses while maintaining backward compatibility, the version v1 of the API is currently in progress of implementation. Some new features can also be added for the new version (ex: pagination of long responses).
New endpoints added to the API must have both routes (`/v0/<endpoints>` et `/v1/<endpoints>`)

## Response template

- GET /v1/<endpoints>:
```
{
    "endpoints": [
        {
            "...": "..."
        }
    ]
}
```

If lots of elements expected in the response:
```
{
    "pagination": {
        "current_page": ...,
        "items_per_page": ...,
        "total_items": ...
    },
     "endpoints": [
        {
            "...": "..."
        }
    ]
}
```

- GET /v1/<endpoint>/<id>:
```
{
    "endpoints": [
        {
            "...": "..."
        }
    ]
}
```

- POST /v1/<endpoints>:  
Used to create a new element of the endpoint. Returned status code should **201**.
```
{
    "endpoint": [
        {
            "...": "..."
        }
    ]
}
```

- PUT /v1/<endpoints>:  
Used to update a new element of the endpoint. Returned status code should **200**.  
Note: If the POST method isn't available for an endpoint, PUT can be used to create a new element. In this case, the returned status code should be **200**
```
{
    "endpoint": [
        {
            "...": "..."
        }
    ]
}
```