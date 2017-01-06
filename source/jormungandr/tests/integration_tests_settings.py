# encoding: utf-8
import os

START_MONITORING_THREAD = False

SAVE_STAT = True

DISABLE_DATABASE = True
# for tests we want only 1/2 seconds timeout instead of the normal 10s
INSTANCE_TIMEOUT = int(os.environ.get('CUSTOM_INSTANCE_TIMEOUT', 500))

# do not authenticate for tests
PUBLIC = True

LOGGER = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters':{
        'default': {
            'format': '[%(asctime)s] [%(levelname)5s] [%(process)5s] [%(name)10s] %(message)s',
        },
    },
    'handlers': {
        'default': {
            'level': 'INFO',
            'class': 'logging.StreamHandler',
            'formatter': 'default',
        },
    },
    'loggers': {
        '': {
            'handlers': ['default'],
            'level': 'INFO',
            'propagate': True
        },
        'navitiacommon.default_values': {
            'handlers': ['default'],
            'level': 'ERROR',
            'propagate': True
        },
    }
}

CACHE_CONFIGURATION = {
    'CACHE_TYPE': 'null'
}

# List of enabled modules
MODULES = {
    'v1': {  # API v1 of Navitia
        'import_path': 'jormungandr.modules.v1_routing.v1_routing',
        'class_name': 'V1Routing'
    }
}

# circuit breaker parameters, for the tests by default we don't want the circuit breaker
CIRCUIT_BREAKER_MAX_INSTANCE_FAIL = 99999
CIRCUIT_BREAKER_INSTANCE_TIMEOUT_S = 1

GRAPHICAL_ISOCHRONE = True
HEAT_MAP = True

PATCH_WITH_GEVENT_SOCKET = True
