# encoding: utf-8
START_MONITORING_THREAD = False

SAVE_STAT = True

DISABLE_DATABASE = True

INSTANCE_TIMEOUT = 500  # for tests we want only 1/2 seconds timeout instead of the normal 10s timeout

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

# Places configuration
AUTOCOMPLETE = {
    # Kraken API
    "class": "jormungandr.interfaces.v1.Places.Places"
    # Elasticsearch API
    #"class": "jormungandr.interfaces.v1.Places.WorldWidePlaces",
    #'args': {
    #    "hosts": ["http://127.0.0.1:9200"],
    #    "user": "",
    #    "password": "",
    #    "use_ssl": False
    #}
}
