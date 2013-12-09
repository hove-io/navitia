#encoding: utf-8
import logging
from datetime import timedelta

CELERY_BROKER_URL = 'amqp://guest:guest@localhost:5672//'

CELERY_ACCEPT_CONTENT = ['pickle', 'json']


CELERYBEAT_SCHEDULE = {
    'scan-every-30-seconds': {
        'task': 'pyed.tasks.scan',
        'schedule': timedelta(seconds=30),
        'options': {'expires': 30}
    },
}

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
LOG_FILENAME = "/tmp/pyed.log"

#nivrau de log de l'orm
#INFO => requete executé
#DEBUG => requete executé + résultat
LOG_LEVEL_SQLALCHEMY = logging.WARN

REDIS_HOST = 'localhost'

REDIS_PORT = 6379

#indice de la base de données redis utilisé, entier de 0 à 15 par défaut
REDIS_DB = 0

REDIS_PASSWORD = None
