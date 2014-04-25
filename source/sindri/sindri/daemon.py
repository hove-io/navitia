# encoding: utf-8

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

from sindri.config import Config
import logging
import sindri.task_pb2
import google
from sindri.saver import EdRealtimeSaver, TechnicalError, FunctionalError
import sys
import time
import kombu
from kombu.mixins import ConsumerMixin


class Sindri(ConsumerMixin):

    """
    this is the service who handle the persistence of realtime events send to
    rabbitmq
    Sindri and Brokk are smiths dwarfs brothers in the nordic mythology

    usage:
    sindri = Sindri()
    sindri.init(conf_filename)
    sindri.run()
    """

    def __init__(self):
        self.connection = None
        self.exchange = None
        self.queues = []
        self.ed_realtime_saver = None
        self.config = Config()

    def init(self, filename):
        """
        init the service with the configuration file taken in parameter
        """
        self.config.load(filename)
        self._init_logger(self.config.log_file, self.config.log_level)
        self.ed_realtime_saver = EdRealtimeSaver(self.config)
        self._init_rabbitmq()


    def _init_logger(self, filename='', level='debug'):
        """
        initialise loggers, by default to debug level and with output on stdout
        """
        level = getattr(logging, level.upper(), logging.DEBUG)
        logging.basicConfig(filename=filename, level=level)

        if level == logging.DEBUG:
            # if we are in debug we log all sql request and results
            logging.getLogger('sqlalchemy.engine').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.pool').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.dialects.postgresql')\
                .setLevel(logging.INFO)

    def _init_rabbitmq(self):
        """
        connect to rabbitmq and init the queues
        """
        self.connection = kombu.Connection(self.config.broker_url)
        instance_name = self.config.instance_name
        exchange_name = self.config.exchange_name
        exchange = kombu.Exchange(exchange_name, 'topic', durable=True)

        logging.getLogger('sindri').info("listen following topics: %s",
                                         self.config.rt_topics)

        for topic in self.config.rt_topics:
            queue_name = instance_name + '_sindri_' + topic
            queue = kombu.Queue(queue_name, exchange=exchange, durable=True,
                                routing_key=topic)
            self.queues.append(queue)

    def get_consumers(self, Consumer, channel):
        return [Consumer(queues=self.queues, callbacks=[self.process_task])]

    def handle_message(self, task):
        if task.message.IsInitialized():
            try:
                self.ed_realtime_saver.persist_message(task.message)
            except FunctionalError as e:
                logging.getLogger('sindri').warn("%s", str(e))
        else:
            logging.getLogger('sindri').warn("message task whitout"
                                             "message in payload")

    def handle_at_perturbation(self, task):
        if task.at_perturbation.IsInitialized():
            try:
                self.ed_realtime_saver.persist_at_perturbation(
                    task.at_perturbation)
            except FunctionalError as e:
                logging.getLogger('sindri').warn("%s", str(e))
        else:
            logging.getLogger('sindri').warn("at perturbation task whitout "
                                             "payload")

    def process_task(self, body, message):
        logging.getLogger('sindri').debug("Message received")
        task = sindri.task_pb2.Task()
        try:
            task.ParseFromString(body)
            logging.getLogger('sindri').debug('%s', str(task))
        except google.protobuf.message.DecodeError as e:
            logging.getLogger('sindri').warn("message is not a valid "
                                             "protobuf task: %s", str(e))
            message.ack()
            return

        try:
            if(task.action == sindri.task_pb2.MESSAGE):
                self.handle_message(task)
            elif(task.action == sindri.task_pb2.AT_PERTURBATION):
                self.handle_at_perturbation(task)

            message.ack()
        except TechnicalError:
            # on technical error (like a database KO) we retry this task later
            # and we sleep 10 seconds
            message.requeue()
            time.sleep(10)
        except:
            logging.exception('fatal')
            sys.exit(1)

    def __del__(self):
        self.close()

    def close(self):
        if self.connection and self.connection.connected:
            self.connection.release()
