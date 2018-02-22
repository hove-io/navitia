# encoding: utf-8
from __future__ import absolute_import, print_function, unicode_literals, division
# emplacement ou charger les fichier de configuration par instances
INSTANCES_DIR = '/etc/jormungandr.d'

# chaine de connnection à postgresql pour la base jormungandr
SQLALCHEMY_DATABASE_URI = 'postgresql://navitia:navitia@localhost/jormun'

# désactivation de l'authentification
PUBLIC = True
