
# Jormungandr
Web service around the c++ core(s). This is the navitia api front end, based on flask.

:snake: **python module**

## Norse mythology
In the Norse mythology, Jörmungandr is a sea serpent that grew so large that it can surround the earth, hence his name: Midgard Serpent. He will be killed by Thor during Ragnarok.

# Setup
## VirtualEnv

```sh
cd navitia/source/jormungandr
mkvirtualenv jormungandr
pip install -r requirements.txt -r requirements_dev.txt -U
```

# Configure

Jormungandr uses 2 different config files:
* `jormungandr.json` : that describes the Kraken's instances and coverage
* `custom_settings.py` : a python script that overwrites default settings

## jormungandr.json

Create a file `jormungandr.json` in a custom directory like:

```json
{
    "key": "some_region",
    "socket": "ipc:///tmp/default_kraken"
}
```

You can copy the one from [documentation/examples/config/Jormungandr.json](https://github.com/CanalTP/navitia/blob/dev/documentation/examples/config/Jormungandr.json)

Make sure `key` and `socket` respectively match `instance_name` and `zmp_socket` from your Kraken config file ([kraken.ini](https://github.com/CanalTP/navitia/blob/dev/documentation/examples/config/kraken.ini)).

## custom_settings.py [optional]

This helps you overwrite the default settings if needed (from [default_settings.py](https://github.com/CanalTP/navitia/blob/dev/source/jormungandr/jormungandr/default_settings.py)).

# Run 

To run the web service, you'll need to:

* set `JORMUNGANDR_INSTANCES_DIR` to point at the directory that contains your `jormungandr.json`.
* add `navitia/source/jormungandr` and `navitia/navitiacommon` to your python path via `PYTHONPATH`.
* [optionaly] set `JORMUNGANDR_CONFIG_FILE` to point at your `custom_settings.py`.  

## Example

From `navitia/source/jormungandr/jormungandr` run :

```python
PYTHONPATH=..:../../navitiacommon/ JORMUNGANDR_INSTANCES_DIR=~/jormung_conf/ JORMUNGANDR_CONFIG_FILE=~/jormung_conf/jormung_settings.py  python manage.py runserver
```

# Troubleshooting

### Python error : `No module named jormungandr`
> from jormungandr import app
>
> ImportError: No module named jormungandr

Make sure you have `navitia/source/jormungandr` in your `PYTHONPATH`

### Python error : `No module named navitiacommon`
> from navitiacommon import response_pb2, type_pb2
>
> ImportError: No module named navitiacommon

Make sure you have `navitia/source/navitiacommon` in your `PYTHONPATH`

### Jormungandr returns a `No region nor coordinates given`
> http://localhost:5000/v1/coverage/
>
> {"message":"No region nor coordinates given","id":"unknown_object"}}

You file `jormungandr.json` is not found or your `key` and `socket` don't match your `kraken.ini`

### Python error : `No module named flask`
> from flask import Flask, got_request_exception
> 
> ImportError: No module named flask

Make sure you use your `jormungandr` VirtualEnv with :

```sh
$ workon jormung
```

### sqlalchemy.exc.OperationalError

Jormungandr is trying to access database "jormungandr". You can turn off this default option from your `custom_settings.py` with :

```python
# encoding: utf-8

import os
from flask_restful.inputs import boolean

DISABLE_DATABASE = boolean(os.getenv('JORMUNGANDR_DISABLE_DATABASE', True))
```