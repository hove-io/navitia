#encoding: utf-8
import logging
from datetime import timedelta

CELERY_BROKER_URL = 'amqp://guest:guest@localhost:5672//'

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

SQLALCHEMY_DATABASE_URI = 'postgresql://navitia:navitia@localhost/jormun'

CELERY_TIMEZONE = 'UTC'

#chaine de connnection à postgresql pour la base jormungandr
#PG_CONNECTION_STRING = 'postgresql://navitia:navitia@localhost/jormun'

INSTANCES_DIR = '..'

#les niveau de log possibles sont:
# - DEBUG
# - INFO
# - WARN
# - ERROR

#niveau de log de l'application
LOG_LEVEL = logging.INFO
#ou sont écrit les logs de l'application
LOG_FILENAME = "/tmp/tyr.log"

#nivrau de log de l'orm
#INFO => requete executé
#DEBUG => requete executé + résultat
LOG_LEVEL_SQLALCHEMY = logging.WARN

REDIS_HOST = 'localhost'

REDIS_PORT = 6379

#indice de la base de données redis utilisé, entier de 0 à 15 par défaut
REDIS_DB = 0

REDIS_PASSWORD = None
