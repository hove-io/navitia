# encoding: utf-8
START_MONITORING_THREAD = False

SAVE_STAT = True

DISABLE_DATABASE = True

# désactivation de l'authentification
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
