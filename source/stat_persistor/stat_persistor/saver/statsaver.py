# encoding: utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

import logging
import datetime
from sqlalchemy import Column, Table, MetaData, select, create_engine, \
    ForeignKey, bindparam, and_, or_, exc
import sqlalchemy
from stat_persistor.saver.stat_request import persist_stat_request
from stat_persistor.saver.utils import FunctionalError, TechnicalError
from stat_persistor.config import Config


class StatSaver(object):
    """
    Class to consume stat elements from rabbitMQ and to save them
    in a database PostGRE using SQLAlChemy.
    """

    def __init__(self, config):
        self.__engine = create_engine(config.stat_connection_string)
        self.meta = MetaData(self.__engine)
        self.request_table = Table('requests', self.meta, autoload=True,
                                   schema='stat')

        self.coverage_table = Table('coverages', self.meta,
                                    autoload=True,
                                    schema='stat')

        self.parameter_table = Table('parameters', self.meta,
                                     autoload=True,
                                     schema='stat')

        self.error_table = Table('errors', self.meta,
                                     autoload=True,
                                     schema='stat')

        self.journey_table = Table('journeys', self.meta,
                                   autoload=True,
                                   schema='stat')

        self.journey_section_table = Table('journey_sections', self.meta,
                                           autoload=True,
                                           schema='stat')

        self.interpreted_parameters_table = Table('interpreted_parameters', self.meta,
                                     autoload=True,
                                     schema='stat')

    def persist_stat(self, config, stat_request):
        self.__persist(config, stat_request, persist_stat_request)

    def __persist(self, config, item, callback):
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
        logger = logging.getLogger('stat_persistor')
        conn = None
        try:
            conn = self.__engine.connect()
            transaction = conn.begin()
        except sqlalchemy.exc.SQLAlchemyError as e:
            logger.exception('error during transaction')
            raise TechnicalError('problem with databases: ' + str(e))

        try:
            callback(self.meta, conn, item)
            transaction.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.exc.DataError) as e:
            logger.exception('error during transaction')
            transaction.rollback()
            raise FunctionalError(str(e))
        except sqlalchemy.exc.SQLAlchemyError as e:
            logger.exception('error during transaction')
            if not hasattr(e, 'connection_invalidated') \
                    or not e.connection_invalidated:
                transaction.rollback()
            raise TechnicalError('problem with databases: ' + str(e))
        except:
            logger.exception('error during transaction')
            try:
                transaction.rollback()
            except:
                pass
            raise TechnicalError('problem the rollback')
        finally:
            if conn:
                conn.close()

