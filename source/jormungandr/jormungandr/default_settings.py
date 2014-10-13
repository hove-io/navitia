# encoding: utf-8

import logging

# path of the configuration file for each instances
INSTANCES_DIR = '/etc/jormungandr.d'

# Start the thread at startup, True in production, False for test environments
START_MONITORING_THREAD = True

#URI for postgresql
# postgresql://<user>:<password>@<host>:<port>/<dbname>
#http://docs.sqlalchemy.org/en/rel_0_9/dialects/postgresql.html#psycopg2
SQLALCHEMY_DATABASE_URI = 'postgresql://navitia:navitia@localhost/jormun'

# disable authentication
PUBLIC = True

#message returned on authentication request
HTTP_BASIC_AUTH_REALM = 'Navitia.io'

# logger configuration
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
            'level': 'DEBUG',
            'class': 'logging.StreamHandler',
            'formatter': 'default',
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

#Parameters for statistics
SAVE_STAT = False
BROKER_URL = 'amqp://guest:guest@localhost:5672//'
EXCHANGE_NAME = 'stat_persistor_exchange'

#Cache configuration, see https://pythonhosted.org/Flask-Cache/ for more information
CACHE_CONFIGURATION = {
    'CACHE_TYPE': 'simple',
    'TIMEOUT_PTOBJECTS': 600,
    'TIMEOUT_AUTHENTICATION': 600,
    'TIMEOUT_PARAMS': 600,
}
