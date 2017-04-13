# encoding: utf-8

import os
import json
from flask_restful.inputs import boolean

# path of the configuration file for each instances
INSTANCES_DIR = os.getenv('JORMUNGANDR_INSTANCES_DIR', '/etc/jormungandr.d')

# Start the thread at startup, True in production, False for test environments
START_MONITORING_THREAD = boolean(os.getenv('JORMUNGANDR_START_MONITORING_THREAD', True))

#URI for postgresql
# postgresql://<user>:<password>@<host>:<port>/<dbname>
#http://docs.sqlalchemy.org/en/rel_0_9/dialects/postgresql.html#psycopg2
SQLALCHEMY_DATABASE_URI = os.getenv('JORMUNGANDR_SQLALCHEMY_DATABASE_URI', 'postgresql://navitia:navitia@localhost/jormungandr')

DISABLE_DATABASE = boolean(os.getenv('JORMUNGANDR_DISABLE_DATABASE', False))

# disable authentication
PUBLIC = boolean(os.getenv('JORMUNGANDR_IS_PUBLIC', True))

#message returned on authentication request
HTTP_BASIC_AUTH_REALM = os.getenv('JORMUNGANDR_HTTP_BASIC_AUTH_REALM', 'Token Required')

from jormungandr.logging_utils import IdFilter

log_level = os.getenv('JORMUNGANDR_LOG_LEVEL', 'DEBUG')
log_format = os.getenv('JORMUNGANDR_LOG_FORMAT', '[%(asctime)s] [%(request_id)s] [%(levelname)5s] [%(process)5s] [%(name)10s] %(message)s')

# logger configuration
LOGGER = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters':{
        'default': {
            'format': log_format,
        },
    },
    'filters': {
        'IdFilter': {
            '()': IdFilter,
        }
    },
    'handlers': {
        'default': {
            'level': log_level,
            'class': 'logging.StreamHandler',
            'formatter': 'default',
            'filters': ['IdFilter'],
        },
    },
    'loggers': {
        '': {
            'handlers': ['default'],
            'level': log_level,
            'propagate': True
        },
    }
}

# Bike self-service configuration
# This should be moved in a central configuration system like ectd, consul, etc...
BSS_PROVIDER = []
for key, value in os.environ.items():
    if key.startswith('JORMUNGANDR_BSS_PROVIDER_'):
        BSS_PROVIDER.append(json.loads(value))

#Parameters for statistics
SAVE_STAT = boolean(os.getenv('JORMUNGANDR_SAVE_STAT', False))
BROKER_URL = os.getenv('JORMUNGANDR_BROKER_URL', 'amqp://guest:guest@localhost:5672//')
EXCHANGE_NAME = os.getenv('JORMUNGANDR_EXCHANGE_NAME', 'stat_persistor_exchange')

#Cache configuration, see https://pythonhosted.org/Flask-Cache/ for more information
default_cache = {
    'CACHE_TYPE': 'null',  # by default cache is not activated
    'TIMEOUT_PTOBJECTS': 600,
    'TIMEOUT_AUTHENTICATION': 600,
    'TIMEOUT_PARAMS': 600,
    'TIMEOUT_TIMEO': 60,
    'TIMEOUT_SYNTHESE': 30,
}

CACHE_CONFIGURATION = json.loads(os.getenv('JORMUNGANDR_CACHE_CONFIGURATION', '{}')) or default_cache

# List of enabled modules
MODULES = {
    'v1': {  # API v1 of Navitia
        'import_path': 'jormungandr.modules.v1_routing.v1_routing',
        'class_name': 'V1Routing'
    }
}

# This should be moved in a central configuration system like ectd, consul, etc...
AUTOCOMPLETE_SYSTEMS = json.loads(os.getenv('JORMUNGANDR_AUTOCOMPLETE_SYSTEMS', '{}')) or None

ISOCHRONE_DEFAULT_VALUE = os.getenv('JORMUNGANDR_ISOCHRONE_DEFAULT_VALUE', 1800) # in s

# circuit breaker parameters.
CIRCUIT_BREAKER_MAX_INSTANCE_FAIL = 4  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_INSTANCE_TIMEOUT_S = 60  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_TIMEO_FAIL = 4  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_TIMEO_TIMEOUT_S = 60  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_SYNTHESE_FAIL = 4  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_SYNTHESE_TIMEOUT_S = 60  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_JCDECAUX_FAIL = 4  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_JCDECAUX_TIMEOUT_S = 60  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_CLEVERAGE_FAIL = 4  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_CLEVERAGE_TIMEOUT_S = 60  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_VALHALLA_FAIL = 4  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_VALHALLA_TIMEOUT_S = 60  # the circuit breaker retries after this timeout (in seconds)

CIRCUIT_BREAKER_MAX_GEOVELO_FAIL = 4  # max instance call failures before stopping attempt
CIRCUIT_BREAKER_GEOVELO_TIMEOUT_S = 60  # the circuit breaker retries after this timeout (in seconds)

# Default region instance
# DEFAULT_REGION = 'default'


GRAPHICAL_ISOCHRONE = boolean(os.getenv('JORMUNGANDR_GRAPHICAL_ISOCHRONE', False))
HEAT_MAP = boolean(os.getenv('JORMUNGANDR_HEAT_MAP', False))
# This parameter are used to apply gevent's monkey patch
# The Goal is to activate parallel calling valhalla, without the patch, parallel http calling may not work
PATCH_WITH_GEVENT_SOCKET = bool(os.getenv('JORMUNGANDR_PATCH_WITH_GEVENT_SOCKET', False))

GREENLET_POOL_SIZE = int(os.getenv('JORMUNGANDR_GEVENT_POOL_SIZE', 10))

USE_SERPY = boolean(os.getenv('JORMUNGANDR_USE_SERPY', False))
