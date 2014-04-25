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

#encoding: utf-8
from connectors.config import Config
from connectors.at.selector.atreader import AtRealtimeReader
import logging
import kombu
import connectors.task_pb2


class ConnectorAT(object):
    def __init__(self):
        self.connection = None
        self.producer = None
        self.at_realtime_reader = None
        self.config = Config()

    def init(self, filename):
        """
        initialize the service with the configuration file taken in parameters
        """
        self.config.load(filename)
        self._init_logger(self.config.logger_file, self.config.logger_level)
        self.at_realtime_reader = AtRealtimeReader(self.config)
        self._init_rabbitmq()

    def _init_logger(self, filename='/tmp/connector.log', level='debug'):
        """
        initialise loggers, by default to debug level and with output on stdout
        """
        level = getattr(logging, level.upper(), logging.DEBUG)
        logging.basicConfig(filename=filename, level=level)

        if level == logging.DEBUG:
            #on active les logs de sqlalchemy si on est en debug:
            #log des requetes et des resultats
            logging.getLogger('sqlalchemy.engine').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.pool').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.dialects.postgresql') \
                .setLevel(logging.INFO)


    def _init_rabbitmq(self):
        """
        initialize the rabbitmq connection and setup the producer
        """
        self.connection = kombu.Connection(self.config.broker_url)
        exchange_name = self.config.exchange_name
        exchange = kombu.Exchange(exchange_name, 'topic', durable=True)
        self.producer = self.connection.Producer(exchange=exchange)

    def run(self):
        self.at_realtime_reader.execute()
        logging.getLogger('connector').info("put %i message and %i disruptions"
                    "to following topics: %s" %\
                    (len(self.at_realtime_reader.message_list),
                     len(self.at_realtime_reader.perturbation_list),
                     self.config.rt_topic))
        for message in self.at_realtime_reader.message_list:
            task = connectors.task_pb2.Task()
            task.action = connectors.task_pb2.MESSAGE
            task.message.MergeFrom(message)
            self.producer.publish(task.SerializeToString(),
                                  routing_key=self.config.rt_topic)

        for perturbation in self.at_realtime_reader.perturbation_list:
            task = connectors.task_pb2.Task()
            task.action = connectors.task_pb2.AT_PERTURBATION
            task.at_perturbation.MergeFrom(perturbation)

            self.producer.publish(task.SerializeToString(),
                                  routing_key=self.config.rt_topic)
