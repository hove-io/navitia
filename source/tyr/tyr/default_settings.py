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

#log level for the app
LOG_LEVEL = logging.INFO
#where are writen the log
LOG_FILENAME = "/tmp/tyr.log"

#log level of the orm
#INFO => log all request executed
#DEBUG => log all request executed and the result
LOG_LEVEL_SQLALCHEMY = logging.WARN

REDIS_HOST = 'localhost'

REDIS_PORT = 6379

#index of the database use in redis, between 0 and 15 by default
REDIS_DB = 0

REDIS_PASSWORD = None

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
