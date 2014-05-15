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

from stat_persistor.config import Config
import logging
import navitiacommon.stat_pb2
import google.protobuf.message import DecodeError
from stat_persistor.saver.statsaver import StatSaver
from stat_persistor.saver.utils import TechnicalError, FunctionalError
import sys
import time
import kombu
from kombu.mixins import ConsumerMixin


class StatPersistor(ConsumerMixin):

    """
    this is the service who handle the persistence of statistiques send by
    rabbitmq
    """

    def __init__(self, conf_file):
        self.connection = None
        self.exchange = None
        self.queues = []
        self.config = Config()

        self.config.load(conf_file)
        self.stat_saver = StatSaver(self.config)
        self._init_rabbitmq()

    def _init_rabbitmq(self):
        """
        connect to rabbitmq and init the queues
        """
        self.connection = kombu.Connection(self.config.broker_url)
        exchange_name = self.config.exchange_name
        exchange = kombu.Exchange(exchange_name, type="direct")
        logging.getLogger('stat_persistor').info("listen following exchange: %s",
                                         self.config.exchange_name)

        queue_name = self.config.queue_name
        queue = kombu.Queue(queue_name, exchange=exchange, durable=True)
        self.queues.append(queue)

    def get_consumers(self, Consumer, channel):
        return [Consumer(queues=self.queues, callbacks=[self.process_task])]

    def handle_statistics(self, stat_hit):
        if stat_hit.IsInitialized():
            try:
                self.stat_saver.persist_stat(self.config, stat_hit)
            except (FunctionalError) as e:
                logging.getLogger('stat_persistor').warn("error while preparing stats to save: {}".format(str(e)))
        else:
            logging.getLogger('stat_persistor').warn("protobuff query not initialized,"
                                                     " no stat logged")

    def process_task(self, body, message):
        logging.getLogger('stat_persistor').debug("Message received")
        stat_request = navitiacommon.stat_pb2.StatRequest()
        try:
            stat_request.ParseFromString(body)
            logging.getLogger('stat_persistor').debug('query received: {}'.format(str(stat_request)))
        except DecodeError as e:
            logging.getLogger('stat_persistor').warn("message is not a valid "
                                             "protobuf task: {}".format(str(e)))
            message.ack()
            return

        try:
            self.handle_statistics(stat_request)
            message.ack()
        except (TechnicalError) as e:
                logging.getLogger('stat_persistor').warn("error while saving stats: {}".format(str(e)))
                # on technical error (like a database KO) we retry this task later
                # and we sleep 10 seconds
                message.requeue()
                time.sleep(10)

    def __del__(self):
        self.close()

    def close(self):
        if self.connection and self.connection.connected:
            self.connection.release()
