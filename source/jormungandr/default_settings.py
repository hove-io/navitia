#encoding: utf-8
#emplacement ou charger les fichier de configuration par instances
INSTANCES_DIR = '/etc/jormungandr.d'

#chaine de connnection à postgresql pour la base jormungandr
PG_CONNECTION_STRING = 'postgresql://navitia:navitia@localhost/jormun'

#désactivation de l'authentification
PUBLIC = True

REDIS_HOST = 'localhost'

REDIS_PORT = 6379

REDIS_DB = 0

REDIS_PASSWORD = None
