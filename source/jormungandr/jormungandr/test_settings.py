# encoding: utf-8

from __future__ import absolute_import
import logging

# emplacement ou charger les fichier de configuration par instances
INSTANCES_DIR = '/etc/jormungandr.d'

# Start the thread at startup, True in production, False for test environments
START_MONITORING_THREAD = False

# chaine de connnection à postgresql pour la base jormungandr
SQLALCHEMY_DATABASE_URI = 'postgresql://navitia:navitia@localhost/jormun_test'

# désactivation de l'authentification
PUBLIC = True

REDIS_HOST = 'localhost'

REDIS_PORT = 6379

# indice de la base de données redis utilisé, entier de 0 à 15 par défaut
REDIS_DB = 0

REDIS_PASSWORD = None

# Desactive l'utilisation du cache, et donc de redis
CACHE_DISABLED = False

# durée de vie des info d'authentification dans le cache en secondes
AUTH_CACHE_TTL = 300

ERROR_HANDLER_FILE = 'jormungandr.log'
ERROR_HANDLER_TYPE = 'rotating'  # can be timedrotating
ERROR_HANDLER_PARAMS = {'maxBytes': 20000000, 'backupCount': 5}
LOG_LEVEL = logging.DEBUG

# This parameter is used to apply gevent's monkey patch
# The Goal is to activate parallel calling valhalla, without the patch, parallel http and https calling may not work
PATCH_WITH_GEVENT_SOCKET = True
