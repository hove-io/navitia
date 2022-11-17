# encoding: utf-8
from datetime import timedelta
from celery import schedules
import os
from flask_restful.inputs import boolean

# URL for the brokker, by default it's the local rabbitmq
# For amqp (rabbitMQ) the syntax is:
# amqp://<user>:<password>@<host>:<port>/<vhost>
# the default vhost is "/" so the URL end with *two* slash
# http://docs.celeryproject.org/en/latest/configuration.html#std:setting-BROKER_URL
CELERY_BROKER_URL = os.getenv('TYR_CELERY_BROKER_URL', 'amqp://guest:guest@localhost:5672//')
KRAKEN_BROKER_URL = os.getenv('TYR_KRAKEN_BROKER_URL', 'amqp://guest:guest@localhost:5672//')

# URI for postgresql
# postgresql://<user>:<password>@<host>:<port>/<dbname>
# http://docs.sqlalchemy.org/en/rel_0_9/dialects/postgresql.html#psycopg2
SQLALCHEMY_DATABASE_URI = os.getenv(
    'TYR_SQLALCHEMY_DATABASE_URI', 'postgresql://navitia:navitia@localhost/jormungandr'
)


# URI for cities database
# postgresql://<user>:<password>@<host>:<port>/<dbname>
# http://docs.sqlalchemy.org/en/rel_0_9/dialects/postgresql.html#psycopg2
CITIES_DATABASE_URI = os.getenv('TYR_CITIES_DATABASE_URI', 'postgresql://navitia:navitia@localhost/cities')

# Path where the cities osm file will be saved
CITIES_OSM_FILE_PATH = os.getenv('TYR_CITIES_OSM_FILE_PATH', '.')

# Path to the directory where the configuration file of each instance of ed are defined
INSTANCES_DIR = os.getenv('TYR_INSTANCES_DIR', '.')

# Path to the directory where the data sources for autocomplete are stocked
AUTOCOMPLETE_DIR = os.getenv('TYR_AUTOCOMPLETE_DIR', "/srv/ed/autocomplete")

AUOTOCOMPLETE_MAX_BACKUPS_TO_KEEP = os.getenv('TYR_AUOTOCOMPLETE_MAX_BACKUPS_TO_KEEP', 5)

# Max number of dataset to keep per instance and type
DATASET_MAX_BACKUPS_TO_KEEP = os.getenv('TYR_DATASET_MAX_BACKUPS_TO_KEEP', 1)

# Period of time to keep job (in days)
JOB_MAX_PERIOD_TO_KEEP = os.getenv('TYR_JOB_MAX_PERIOD_TO_KEEP', 60)

# Tyr v1: Items per page with pagination
MAX_ITEMS_PER_PAGE = os.getenv('TYR_MAX_ITEMS_PER_PAGE', 10)


# Log Level available
# - DEBUG
# - INFO
# - WARN
# - ERROR

log_level = os.getenv('TYR_LOG_LEVEL', 'DEBUG')

# logger configuration
LOGGER = {
    'version': 1,
    'disable_existing_loggers': True,
    'formatters': {
        'default': {'format': '[%(asctime)s] [%(levelname)5s] [%(process)5s] [%(name)25s] %(message)s'},
        'instance': {
            'format': '%(name)s: [%(asctime)s] [%(levelname)5s] [%(process)5s] [%(task_id)s] %(message)s'
        },
    },
    'handlers': {'default': {'level': log_level, 'class': 'logging.StreamHandler', 'formatter': 'default'}},
    'loggers': {
        '': {'handlers': ['default'], 'level': log_level},
        'celery': {'level': 'INFO'},
        'sqlalchemy.engine': {'handlers': ['default'], 'level': 'WARN', 'propagate': True},
        'sqlalchemy.pool': {'handlers': ['default'], 'level': 'WARN', 'propagate': True},
        'sqlalchemy.dialects.postgresql': {'handlers': ['default'], 'level': 'WARN', 'propagate': True},
    },
}

REDIS_HOST = os.getenv('TYR_REDIS_HOST', '127.0.0.1')

REDIS_PORT = os.getenv('TYR_REDIS_PORT', 6379)

# index of the database use in redis, between 0 and 15 by default
REDIS_DB = os.getenv('TYR_REDIS_DB', 0)

REDIS_PASSWORD = os.getenv('TYR_REDIS_PASSWORD', None)

# Validate the presence of a mx record on the domain
EMAIL_CHECK_MX = os.getenv('TYR_EMAIL_CHECK_MX', 'true').lower() in ['1', 'true', 'yes']

# Validate the email by connecting to the smtp server, but doesn't send an email
EMAIL_CHECK_SMTP = os.getenv('TYR_EMAIL_CHECK_SMTP', 'true').lower() in ['1', 'true', 'yes']

# configuration of celery, don't edit
CELERY_ACCEPT_CONTENT = os.getenv('TYR_CELERY_ACCEPT_CONTENT', ['pickle', 'json'])

CELERYBEAT_SCHEDULE = {
    'udpate-data-every-30-seconds': {
        'task': 'tyr.tasks.update_data',
        'schedule': timedelta(seconds=30),
        'options': {'expires': 25},
    },
    'scan-instances-every-minutes': {
        'task': 'tyr.tasks.scan_instances',
        'schedule': timedelta(minutes=1),
        'options': {'expires': 50},
    },
    'udpate-autocomplete-every-30-seconds': {
        'task': 'tyr.tasks.update_autocomplete',
        'schedule': timedelta(seconds=30),
        'options': {'expires': 25},
    },
    'purge-autocomplete-everyday': {
        'task': 'tyr.tasks.purge_autocomplete',
        'schedule': schedules.crontab(hour=0, minute=0),  # Task is executed daily at midnight
        'options': {'expires': 120},
    },
    'purge-dataset-everyday': {
        'task': 'tyr.tasks.purge_datasets',
        'schedule': schedules.crontab(hour=2, minute=30),  # Task is executed daily at 2H30
        'options': {'expires': 120},
    },
    'purge-old-job-every-week': {
        'task': 'tyr.tasks.purge_jobs',
        'schedule': schedules.crontab(
            hour=1, minute=0, day_of_week='monday'
        ),  # Task is executed at at 1H00 on monday every week
        'options': {'expires': 172800},  # Expires after 2 days (in s)
    },
    'heartbeat-kraken': {
        'task': 'tyr.tasks.heartbeat',
        'schedule': timedelta(minutes=30),
        'options': {'expires': 50},
    },
}
CELERY_TIMEZONE = os.getenv('TYR_CELERY_TIMEZONE', 'UTC')

# http://docs.celeryproject.org/en/master/configuration.html#std:setting-CELERYBEAT_SCHEDULE_FILENAME
CELERYBEAT_SCHEDULE_FILENAME = os.getenv('TYR_CELERYBEAT_SCHEDULE_FILENAME', '/tmp/celerybeat-schedule')

CELERYD_HIJACK_ROOT_LOGGER = os.getenv('TYR_CELERYD_HIJACK_ROOT_LOGGER', False)

MIMIR_URL = os.getenv('TYR_MIMIR_URL', None)

MIMIR7_URL = os.getenv('TYR_MIMIR7_URL', None)

MIMIR_CONFIG_DIR = os.getenv('MIMIR_CONFIG_DIR', "/etc/mimir/")

MIMIR_PLATFORM_TAG = os.getenv('TYR_MIMIR_PLATFORM_TAG', 'default')

MINIO_URL = os.getenv('TYR_MINIO_URL', None)

MINIO_BUCKET_NAME = os.getenv('TYR_MINIO_BUCKET_NAME', None)

MINIO_USE_IAM_PROVIDER = os.getenv('TYR_MINIO_USE_IAM_PROVIDER', 'true').lower() in ['1', 'true', 'yes']

MINIO_ACCESS_KEY = os.getenv('TYR_MINIO_ACCESS_KEY', None)

MINIO_SECRET_KEY = os.getenv('TYR_MINIO_SECRET_KEY', None)

# we don't enable serpy for now
USE_SERPY = os.getenv('TYR_USE_SERPY', False)

# https://flask-sqlalchemy.palletsprojects.com/en/2.x/signals/
# deprecated and slow
SQLALCHEMY_TRACK_MODIFICATIONS = os.getenv('TYR_SQLALCHEMY_TRACK_MODIFICATIONS', False)

# Url of a secondary Tyr. The data posted to an instance of this Tyr via the '/jobs'
# endpoint will be reposted to this url
POST_DATA_TO_TYR = os.getenv('TYR_POST_DATA_TO_TYR', None)

USE_LOCAL_SYS_LOG = os.getenv('TYR_USE_LOCAL_SYS_LOG', 'true').lower() in ['1', 'true', 'yes']

ENABLE_USER_EVENT = boolean(os.getenv('TYR_ENABLE_USER_EVENT', True))
