# encoding: utf-8

from __future__ import absolute_import
import os
import json
from flask_restful.inputs import boolean

# sql queries will return an exception if the query did not succeed before `statement_timeout`
DEFAULT_SQLALCHEMY_ENGINE_OPTIONS = {
    "connect_args": {"options": "-c statement_timeout=1000", "application_name": "Jormungandr"}
}  # 1000ms

# path of the configuration file for each instances
INSTANCES_DIR = os.getenv('JORMUNGANDR_INSTANCES_DIR', '/etc/jormungandr.d')


INSTANCES_TIMEOUT = float(os.getenv('JORMUNGANDR_INSTANCES_TIMEOUT_S', 10))
PLACE_FAST_TIMEOUT = float(os.getenv('JORMUNGANDR_PLACE_FAST_TIMEOUT_S', 1))

# Patern that matches Jormungandr configuration files
#  ex: '*.json' will match all json files within "INSTANCES_DIR" directory
INSTANCES_FILENAME_PATTERN = os.getenv('JORMUNGANDR_INSTANCES_FILENAME_PATTERN', '*.json')

# Start the thread at startup, True in production, False for test environments
START_MONITORING_THREAD = boolean(os.getenv('JORMUNGANDR_START_MONITORING_THREAD', True))

# URI for postgresql
# postgresql://<user>:<password>@<host>:<port>/<dbname>
# http://docs.sqlalchemy.org/en/rel_0_9/dialects/postgresql.html#psycopg2
SQLALCHEMY_DATABASE_URI = os.getenv(
    'JORMUNGANDR_SQLALCHEMY_DATABASE_URI', 'postgresql://navitia:navitia@localhost/jormungandr'
)

DISABLE_DATABASE = boolean(os.getenv('JORMUNGANDR_DISABLE_DATABASE', False))

# Active the asynchronous ridesharing mode
ASYNCHRONOUS_RIDESHARING = boolean(os.getenv('JORMUNGANDR_ASYNCHRONOUS_RIDESHARING', False))
# Active ridesharing service call with async greenlet
GREENLET_POOL_FOR_RIDESHARING_SERVICES = boolean(os.getenv('JORMUNGANDR_GREENLET_POOL_FOR_RIDESHARING', False))
RIDESHARING_GREENLET_POOL_SIZE = int(os.getenv('JORMUNGANDR_RIDESHARING_GREENLET_POOL_SIZE', 10))

# disable authentication
PUBLIC = boolean(os.getenv('JORMUNGANDR_IS_PUBLIC', True))

# message returned on authentication request
HTTP_BASIC_AUTH_REALM = os.getenv('JORMUNGANDR_HTTP_BASIC_AUTH_REALM', 'Token Required')

NEWRELIC_CONFIG_PATH = os.getenv('JORMUNGANDR_NEWRELIC_CONFIG_PATH', None)

from jormungandr.logging_utils import IdFilter

log_level = os.getenv('JORMUNGANDR_LOG_LEVEL', 'DEBUG')
log_format = os.getenv(
    'JORMUNGANDR_LOG_FORMAT',
    '[%(asctime)s] [%(request_id)s] [%(levelname)5s] [%(process)5s] [%(name)10s] %(message)s',
)
log_formatter = os.getenv('JORMUNGANDR_LOG_FORMATTER', 'default')  # default or json
log_extras = json.loads(os.getenv('JORMUNGANDR_LOG_EXTRAS', '{}'))  # fields to add to the logger

access_log_format = os.getenv(
    'JORMUNGANDR_ACCESS_LOG_FORMAT',
    '[%(asctime)s] [%(request_id)s] [%(process)5s] [%(name)10s] %(message)s',
)
access_log_formatter = os.getenv('JORMUNGANDR_ACCESS_LOG_FORMATTER', 'access_log')

# logger configuration
LOGGER = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters': {
        'default': {'format': log_format},
        'access_log': {'format': access_log_format},
        'json': {
            '()': 'jormungandr.logging_utils.CustomJsonFormatter',
            'format': log_format,
            'extras': log_extras,
        },
    },
    'filters': {'IdFilter': {'()': IdFilter}},
    'handlers': {
        'default': {
            'level': log_level,
            'class': 'logging.StreamHandler',
            'formatter': log_formatter,
            'filters': ['IdFilter'],
        },
        'access_log': {
            'level': 'INFO',
            'class': 'logging.StreamHandler',
            'formatter': access_log_formatter,
            'filters': ['IdFilter'],
        },
    },
    'loggers': {
        '': {'handlers': ['default'], 'level': log_level, 'propagate': True},
        'jormungandr.access': {'handlers': ['access_log'], 'level': 'INFO', 'propagate': False},
    },
}

# Bike self-service configuration
# This should be moved in a central configuration system like ectd, consul, etc...
BSS_PROVIDER = json.loads(os.getenv("JORMUNGANDR_BSS_PROVIDER_", "[]"))
# Car parking places availability service
CAR_PARK_PROVIDER = json.loads(os.getenv("JORMUNGANDR_CAR_PARK_PROVIDER_", "[]"))
# Equipment details service configuration
EQUIPMENT_DETAILS_PROVIDERS = json.loads(os.getenv("JORMUNGANDR_EQUIPMENT_DETAILS_PROVIDER_", "[]"))

# Parameters for statistics
SAVE_STAT = boolean(os.getenv('JORMUNGANDR_SAVE_STAT', False))
BROKER_URL = os.getenv('JORMUNGANDR_BROKER_URL', 'amqp://guest:guest@localhost:5672//')
EXCHANGE_NAME = os.getenv('JORMUNGANDR_EXCHANGE_NAME', 'stat_persistor_exchange')
STAT_CONNECTION_TIMEOUT = float(
    os.getenv('JORMUNGANDR_STAT_CONNECTION_TIMEOUT', 1)
)  # connection_timeout in second
# max instance call failures before stopping attempt
STAT_CIRCUIT_BREAKER_MAX_FAIL = int(os.getenv('JORMUNGANDR_STAT_CIRCUIT_BREAKER_MAX_FAIL', 5))
# the circuit breaker retries after this timeout (in seconds)
STAT_CIRCUIT_BREAKER_TIMEOUT_S = int(os.getenv('JORMUNGANDR_STAT_CIRCUIT_BREAKER_TIMEOUT_S', 60))

# Cache configuration, see https://pythonhosted.org/Flask-Caching/ for more information
default_cache = {
    'CACHE_TYPE': 'null',  # by default cache is not activated
    'TIMEOUT_PTOBJECTS': 600,
    'TIMEOUT_AUTHENTICATION': 600,
    'TIMEOUT_PARAMS': 600,
    'TIMEOUT_TIMEO': 60,
    'TIMEOUT_SYNTHESE': 30,
}

CACHE_CONFIGURATION = json.loads(os.getenv('JORMUNGANDR_CACHE_CONFIGURATION', '{}')) or default_cache


default_memory_cache = {
    'CACHE_TYPE': 'null',  # by default cache is not activated
    'TIMEOUT_AUTHENTICATION': 30,
    'TIMEOUT_PARAMS': 30,
}

MEMORY_CACHE_CONFIGURATION = (
    json.loads(os.getenv('JORMUNGANDR_MEMORY_CACHE_CONFIGURATION', '{}')) or default_memory_cache
)

# List of enabled modules
MODULES = {
    'v1': {  # API v1 of Navitia
        'import_path': 'jormungandr.modules.v1_routing.v1_routing',
        'class_name': 'V1Routing',
    }
}

# This should be moved in a central configuration system like ectd, consul, etc...

AUTOCOMPLETE_SYSTEMS = json.loads(os.getenv('JORMUNGANDR_AUTOCOMPLETE_SYSTEMS', '{}')) or None

ISOCHRONE_DEFAULT_VALUE = os.getenv('JORMUNGANDR_ISOCHRONE_DEFAULT_VALUE', 1800)  # in s

# circuit breaker parameters.
CIRCUIT_BREAKER_MAX_INSTANCE_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_INSTANCE_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_INSTANCE_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_INSTANCE_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_TIMEO_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_TIMEO_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_TIMEO_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_TIMEO_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_SYNTHESE_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_SYNTHESE_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_SYNTHESE_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_SYNTHESE_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_JCDECAUX_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_JCDECAUX_FAIL', 4)) # max instance call failures before stopping attempt
CIRCUIT_BREAKER_JCDECAUX_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_JCDECAUX_TIMEOUT_S', 60)) # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_CAR_PARK_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_CAR_PARK_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_CAR_PARK_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_CAR_PARK_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_CLEVERAGE_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_CLEVERAGE_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_CLEVERAGE_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_CLEVERAGE_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_SYTRAL_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_SYTRAL_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_SYTRAL_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_SYTRAL_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_VALHALLA_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_VALHALLA_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_VALHALLA_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_VALHALLA_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_GEOVELO_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_GEOVELO_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_GEOVELO_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_GEOVELO_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_HERE_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_HERE_FAIL', 4)) # max instance call failures before stopping attempt
CIRCUIT_BREAKER_HERE_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_HERE_TIMEOUT_S', 60)) # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_CYKLEO_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_CYKLEO_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_CYKLEO_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_CYKLEO_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_INSTANT_SYSTEM_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_INSTANT_SYSTEM_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_INSTANT_SYSTEM_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_INSTANT_SYSTEM_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_BLABLALINES_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_BLABLALINES_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_BLABLALINES_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_BLABLALINES_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_KAROS_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_KAROS_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_KAROS_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_KAROS_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_BRAGI_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_BRAGI_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_BRAGI_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_BRAGI_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_ASGARD_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_ASGARD_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_ASGARD_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_ASGARD_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_KLAXIT_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_KLAXIT_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_KLAXIT_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_KLAXIT_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_OUESTGO_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_OUESTGO_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_OUESTGO_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_OUESTGO_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_FORSETI_FAIL = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_MAX_FORSETI_FAIL', 4))  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_FORSETI_TIMEOUT_S = int(os.getenv('JORMUNGANDR_CIRCUIT_BREAKER_FORSETI_TIMEOUT_S', 60))  # the circuit breaker retries after this timeout (in seconds)


# Default region instance
# DEFAULT_REGION = 'default'


GRAPHICAL_ISOCHRONE = boolean(os.getenv('JORMUNGANDR_GRAPHICAL_ISOCHRONE', True))
HEAT_MAP = boolean(os.getenv('JORMUNGANDR_HEAT_MAP', True))

# These parameters are used to apply gevent's monkey patch
# The Goal is to activate parallel calling valhalla, without the patch, parallel http and https calling may not work
PATCH_WITH_GEVENT_SOCKET = bool(os.getenv('JORMUNGANDR_PATCH_WITH_GEVENT_SOCKET', True))

GREENLET_POOL_SIZE = int(os.getenv('JORMUNGANDR_GEVENT_POOL_SIZE', 10))

PARSER_MAX_COUNT = int(os.getenv('JORMUNGANDR_PARSER_MAX_COUNT', 1000))

if boolean(os.getenv('JORMUNGANDR_DISABLE_SQLPOOLING', False)):
    from sqlalchemy.pool import NullPool

    SQLALCHEMY_POOLCLASS = NullPool


SQLALCHEMY_ENGINE_OPTIONS = (
    json.loads(os.getenv('JORMUNGANDR_SQLALCHEMY_ENGINE_OPTIONS', '{}')) or DEFAULT_SQLALCHEMY_ENGINE_OPTIONS
)

MAX_JOURNEYS_CALLS = int(os.getenv('JORMUNGANDR_MAX_JOURNEYS_CALLS', 20))
DEFAULT_AUTOCOMPLETE_BACKEND = os.getenv('JORMUNGANDR_DEFAULT_AUTOCOMPLETE_BACKEND', 'bragi')


# ZMQ
ZMQ_DEFAULT_SOCKET_TYPE = os.getenv('JORMUNGANDR_ZMQ_DEFAULT_SOCKET_TYPE', 'persistent')

ZMQ_SOCKET_TTL_SECONDS = float(os.getenv('JORMUNGANDR_ZMQ_SOCKET_TTL_SECONDS', 10))
ASGARD_ZMQ_SOCKET_TTL_SECONDS = float(os.getenv('JORMUNGANDR_ASGARD_ZMQ_SOCKET_TTL_SECONDS', 10))


# Variable used only when deploying on aws
ASGARD_ZMQ_SOCKET = os.getenv('JORMUNGANDR_ASGARD_ZMQ_SOCKET')

# https://flask-sqlalchemy.palletsprojects.com/en/2.x/signals/
# deprecated and slow
SQLALCHEMY_TRACK_MODIFICATIONS = False

BEST_BOARDING_POSITIONS_DIR = os.getenv('JORMUNGANDR_BEST_BOARDING_POSITIONS_DIR', None)
ORIGIN_DESTINATION_DIR = os.getenv('JORMUNGANDR_ORIGIN_DESTINATION_DIR', None)
OLYMPIC_SITE_PARAMS_DIR = os.getenv('JORMUNGANDR_OLYMPIC_SITE_PARAMS_DIR', None)
DEPLOYMENT_AZ = os.getenv('JORMUNGANDR_DEPLOYMENT_AZ', "unknown")
