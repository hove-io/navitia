#encoding: utf-8
import logging
from datetime import timedelta

#URL for the brokker, by default it's the local rabbitmq
#For amqp (rabbitMQ) the syntax is:
#amqp://<user>:<password>@<host>:<port>/<vhost>
#the default vhost is "/" so the URL end with *two* slash
#http://docs.celeryproject.org/en/latest/configuration.html#std:setting-BROKER_URL
CELERY_BROKER_URL = 'amqp://guest:guest@localhost:5672//'

#URI for postgresql
# postgresql://<user>:<password>@<host>:<port>/<dbname>
#http://docs.sqlalchemy.org/en/rel_0_9/dialects/postgresql.html#psycopg2
SQLALCHEMY_DATABASE_URI = 'postgresql://navitia:navitia@localhost/jormun2'

#Path to the directory where the configuration file of each instance of ed are defined
INSTANCES_DIR = '..'

#Log Level available
# - DEBUG
# - INFO
# - WARN
# - ERROR

# logger configuration
LOGGER = {
    'version': 1,
    'disable_existing_loggers': False,
    'handlers': {
        'default': {
            'level': 'DEBUG',
            'class': 'logging.StreamHandler',
        },
    },
    'loggers': {
        '': {
            'handlers': ['default'],
            'level': 'DEBUG',
            'propagate': True
        },
        'sqlalchemy.engine': {
            'handlers': ['default'],
            'level': 'WARN',
            'propagate': True
        },
        'sqlalchemy.pool': {
            'handlers': ['default'],
            'level': 'WARN',
            'propagate': True
        },
        'sqlalchemy.dialects.postgresql': {
            'handlers': ['default'],
            'level': 'WARN',
            'propagate': True
        },
    }
}

REDIS_HOST = 'localhost'

REDIS_PORT = 6379

#index of the database use in redis, between 0 and 15 by default
REDIS_DB = 0

REDIS_PASSWORD = None

#Validate the presence of a mx record on the domain
EMAIL_CHECK_MX = True

#Validate the email by connecting to the smtp server, but doesn't send an email
EMAIL_CHECK_SMTP = True

#configuration of celery, don't edit
CELERY_ACCEPT_CONTENT = ['pickle', 'json']

CELERYBEAT_SCHEDULE = {
    'udpate-data-every-30-seconds': {
        'task': 'tyr.tasks.update_data',
        'schedule': timedelta(seconds=30),
        'options': {'expires': 25}
    },
    'scan-instances-every-minutes': {
        'task': 'tyr.tasks.scan_instances',
        'schedule': timedelta(minutes=1),
        'options': {'expires': 50}
    },
}
CELERY_TIMEZONE = 'UTC'

#http://docs.celeryproject.org/en/master/configuration.html#std:setting-CELERYBEAT_SCHEDULE_FILENAME
CELERYBEAT_SCHEDULE_FILENAME = '/tmp/celerybeat-schedule'
