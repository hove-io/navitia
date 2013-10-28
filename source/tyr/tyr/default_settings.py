#encoding: utf-8
import logging

#chaine de connnection à postgresql pour la base jormungandr
PG_CONNECTION_STRING = 'postgresql://navitia:navitia@localhost/jormun'


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
