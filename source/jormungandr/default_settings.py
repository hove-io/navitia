#encoding: utf-8
#emplacement ou charger les fichier de configuration par instances
INSTANCES_DIR = '/etc/jormungandr.d'

#chaine de connnection à postgresql pour la base jormungandr
PG_CONNECTION_STRING = 'postgresql://navitia:navitia@localhost/jormun'

#désactivation de l'authentification
PUBLIC = True

ERROR_HANDLER_FILE = 'jormungandr.log'
ERROR_HANDLER_TYPE = 'rotating' # can be timedrotating
ERROR_HANDLER_PARAMS = {'maxBytes' : 20000000, 'backupCount' : 5}
