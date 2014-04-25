# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#   
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#  
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#   
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#    
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#    
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#   
# Stay tuned using
# twitter @navitia 
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

# encoding: utf-8
import logging
import datetime
from sqlalchemy import Table, MetaData, select, create_engine
import sqlalchemy
from sindri.saver.message import persist_message
from sindri.saver.at_perturbation import persist_at_perturbation
from sindri.saver.utils import FunctionalError, TechnicalError


class EdRealtimeSaver(object):

    """
    Classe responsable de l'enregistrement en base de donnée des événements
    temps réel.
    """

    def __init__(self, config):
        self.__engine = create_engine(config.ed_connection_string)
        self.meta = MetaData(self.__engine)
        self.message_table = Table('message', self.meta, autoload=True,
                                   schema='realtime')
        self.localized_message_table = Table('localized_message', self.meta,
                                             autoload=True, schema='realtime')
        self.at_perturbation_table = Table('at_perturbation', self.meta,
                                           autoload=True, schema='realtime')

    def persist_message(self, message):
        self.__persist(message, persist_message)

    def persist_at_perturbation(self, perturbation):
        self.__persist(perturbation, persist_at_perturbation)

    def __persist(self, item, callback):
        """
        fonction englobant toute la gestion d'erreur lié à la base de donnée
        et la gestion de la transaction associé

        :param item l'objet à enregistré
        :param callback fonction charger de l'enregistrement de l'objet
        à proprement parler dont la signature est (meta, conn, item)
        meta etant un objet MetaData
        conn la connection à la base de donnée
        item etant l'objet à enregistrer
        """
        logger = logging.getLogger('sindri')
        conn = None
        try:
            conn = self.__engine.connect()
            transaction = conn.begin()
        except sqlalchemy.exc.SQLAlchemyError as e:
            logger.exception('error durring transaction')
            raise TechnicalError('problem with databases: ' + str(e))

        try:
            callback(self.meta, conn, item)
            transaction.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.exc.DataError) as e:
            logger.exception('error durring transaction')
            transaction.rollback()
            raise FunctionalError(str(e))
        except sqlalchemy.exc.SQLAlchemyError as e:
            logger.exception('error durring transaction')
            if not hasattr(e, 'connection_invalidated') \
                    or not e.connection_invalidated:
                transaction.rollback()
            raise TechnicalError('problem with databases: ' + str(e))
        except:
            logger.exception('error durring transaction')
            try:
                transaction.rollback()
            except:
                pass
            raise
        finally:
            if conn:
                conn.close()
