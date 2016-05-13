# encoding: utf-8

import logging

# path of the configuration file for each instances
INSTANCES_DIR = '/etc/jormungandr.d'

# Start the thread at startup, True in production, False for test environments
START_MONITORING_THREAD = True

#URI for postgresql
# postgresql://<user>:<password>@<host>:<port>/<dbname>
#http://docs.sqlalchemy.org/en/rel_0_9/dialects/postgresql.html#psycopg2
SQLALCHEMY_DATABASE_URI = 'postgresql://navitia:navitia@localhost/jormungandr'

DISABLE_DATABASE = False

# disable authentication
PUBLIC = True

#message returned on authentication request
HTTP_BASIC_AUTH_REALM = 'Token Required'

from jormungandr.logging_utils import IdFilter

# logger configuration
LOGGER = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters':{
        'default': {
            'format': '[%(asctime)s] [%(request_id)s] [%(levelname)5s] [%(process)5s] [%(name)10s] %(message)s',
        },
    },
    'filters': {
        'IdFilter': {
            '()': IdFilter,
        }
    },
    'handlers': {
        'default': {
            'level': 'DEBUG',
            'class': 'logging.StreamHandler',
            'formatter': 'default',
            'filters': ['IdFilter'],
        },
    },
    'loggers': {
        '': {
            'handlers': ['default'],
            'level': 'DEBUG',
            'propagate': True
        },
    }
}

# Bike self-service configuration
BSS_PROVIDER = ()

#Parameters for statistics
SAVE_STAT = False
BROKER_URL = 'amqp://guest:guest@localhost:5672//'
EXCHANGE_NAME = 'stat_persistor_exchange'

#Cache configuration, see https://pythonhosted.org/Flask-Cache/ for more information
CACHE_CONFIGURATION = {
    'CACHE_TYPE': 'null',  # by default cache is not activated
    'TIMEOUT_PTOBJECTS': 600,
    'TIMEOUT_AUTHENTICATION': 600,
    'TIMEOUT_PARAMS': 600,
    'TIMEOUT_TIMEO': 60,
    'TIMEOUT_SYNTHESE': 30,
}

# List of enabled modules
MODULES = {
    'v1': {  # API v1 of Navitia
        'import_path': 'jormungandr.modules.v1_routing.v1_routing',
        'class_name': 'V1Routing'
    }
}

AUTOCOMPLETE = None

ISOCHRONE_DEFAULT_VALUE = 1800  # in s

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

# Default region instance
# DEFAULT_REGION = 'default'

GRAPHICAL_ISOCHRON = False