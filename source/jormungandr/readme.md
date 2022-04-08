
# Jormungandr
Web service around the c++ core(s). This is the navitia api front end, based on Flask.

:snake: **python module**

## Norse mythology
In the Norse mythology, Jörmungandr is a sea serpent that grew so large that it can surround the earth, hence his name: Midgard Serpent. He will be killed by Thor during Ragnarok.

# Setup
## VirtualEnv

```sh
cd navitia/source/jormungandr
mkvirtualenv -r requirements_dev.txt -p /usr/bin/python2 jormungandr
```

# Configure

Jormungandr uses 2 different levels of config:
* `jormungandr.json` : that describes Kraken's instances and coverages
* Jormungandr's core config via :
	* Environment variables [the prefered option]
	* A custom setting file `custom_settings.py` [for advanced usage] - that overwrites default settings

## jormungandr.json

Create a file `jormungandr.json` in a custom directory like:

```json
{
    "key": "some_region",
    "zmq_socket": "ipc:///tmp/default_kraken"
}
```

You can copy the one from [documentation/examples/config/Jormungandr.json](https://github.com/hove-io/navitia/blob/dev/documentation/examples/config/Jormungandr.json)

Make sure `key` and `socket` respectively match `instance_name` and `zmq_socket` from your Kraken config file ([kraken.ini](https://github.com/hove-io/navitia/blob/dev/documentation/examples/config/kraken.ini)).

As an alternative, you can also give the json through an environment variable like:
```sh
JORMUNGANDR_INSTANCE_FR_IDF='{"key": "fr-idf","zmq_socket": "tcp://localhost:3000"}'
```

## Environment Variables

Environment variables can be set when the service is started. Referer to [default_settings.py](https://github.com/hove-io/navitia/blob/dev/source/jormungandr/jormungandr/default_settings.py)

example for a development environment:

```sh
PYTHONPATH=..:../../navitiacommon/ JORMUNGANDR_INSTANCES_DIR=~/jormung_conf/ JORMUNGANDR_START_MONITORING_THREAD=False JORMUNGANDR_DISABLE_DATABASE=True JORMUNGANDR_IS_PUBLIC=True FLASK_APP=jormungandr:app flask run
```

## custom_settings.py

This helps you overwrite the default settings from a file. Prevent from mixing both custom file settings and environment variables.

Use `JORMUNGANDR_CONFIG_FILE` to tell where your file is located like:

```sh
PYTHONPATH=..:../../navitiacommon/ JORMUNGANDR_INSTANCES_DIR=~/jormung_conf/ JORMUNGANDR_CONFIG_FILE=~/jormung_conf/jormung_settings.py FLASK_APP=jormungandr:app flask run
```

# Run

To run the web service, you'll need to:

* set `JORMUNGANDR_INSTANCES_DIR` to point at the directory that contains your `jormungandr.json`.
* add `navitia/source/jormungandr` and `navitia/navitiacommon` to your python path via `PYTHONPATH`.
* [optionally] set `JORMUNGANDR_CONFIG_FILE` to point at your `custom_settings.py`.

## Example

From `navitia/source/jormungandr/jormungandr` run :

```sh
PYTHONPATH=..:../../navitiacommon/ JORMUNGANDR_INSTANCES_DIR=~/jormung_conf/ FLASK_APP=jormungandr:app flask run
```

# Option
## Autocomplete

Setup Jormungandr for using either `Bragi` or `Kraken` for autocompletion

```sh
JORMUNGANDR_AUTOCOMPLETE_SYSTEMS='{"kraken": { "class": "jormungandr.autocomplete.kraken.Kraken" }, "bragi": {"class": "jormungandr.autocomplete.geocodejson.GeocodeJson", "args": {"host": "http://127.0.0.1:4000", "timeout": 2}}}'
```

You can select your autocomplete engine based on the hidden and debug-only `_autocomplete` parameter when querying Jormungandr:

```sh
http://localhost:5000/v1/coverage/default/places?q=rennes&_autocomplete='<kraken|bragi>'
```

It is also possible to set the autocomplete engine for an instance by adding the following line in the config file `jormungandr.json`:

```sh
"default_autocomplete": "<kraken|bragi>"
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

Jormungandr is trying to access a "jormungandr" database. You can disable this default behavior with `JORMUNGANDR_DISABLE_DATABASE=True`.

# How to profile Jormungandr?

The profiling of Jormungandr is done thanks to Werkzeug profiler, for more details, one can read this interesting [article](https://blog.miguelgrinberg.com/post/the-flask-mega-tutorial-part-xvi-debugging-testing-and-profiling), more specifically, in the Profiling section.

To enable the profiling of Jormungandr, one must set `ACTIVATE_PROFILING` to True in the `$JORMUNGANDR_CONFIG_FILE`[default_settings.py](https://github.com/hove-io/navitia/blob/dev/source/jormungandr/jormungandr/default_settings.py).
Then set the correct directory(create one if not exists) for the output of profiler [profile](https://github.com/hove-io/navitia/blob/dev/source/jormungandr/jormungandr/api.py#L130).

The profiling is triggered on every arrival of a request, so a profile file is generated when a request is processed.

To view the generated file, the following packages are needed:
- graphviz: `sudo apt install graphviz`
- gprof2dot: `pip install gprof2dot`


Run the following command to generate and visualize the graph:
 `gprof2dot -f pstats <your_profiling.prof> | dot -Tsvg -o callgraph.svg | eog callgraph.svg&`

Note that, in order to profile the real performance, you should activate the cpp implementation of protobuf by adding the env vars: `PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION_VERSION=2 PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp`.

Have fun!
