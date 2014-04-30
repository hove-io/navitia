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

from flask.ext.script import Command, Option
from navitiacommon import models
from tyr.tasks import reload_at
from tyr import db
import logging
import kombu
from kombu.mixins import ConsumerMixin
from collections import defaultdict
from tyr.helper import load_instance_config
from flask import current_app
from navitiacommon import task_pb2
from datetime import datetime, timedelta

logger = logging.getLogger(__name__)

class AtReloader(ConsumerMixin, Command):
    """run a process who listen to incomming realtime messages from connectors
    and reload appropriate kraken instances"""

    def __init__(self):
        self.topics_to_instances = defaultdict(list)
        self.connection = None
        self.queues = []
        self.last_reload = {}

    def _init(self):
        instances = models.Instance.query.all()
        self.connection = kombu.Connection(
                            current_app.config['CELERY_BROKER_URL'])
        for instance in instances:
            #initialize the last relaod at the minimum date possible
            self.last_reload[instance.id] = datetime(1, 1, 1)
            config = load_instance_config(instance.name)
            exchange = kombu.Exchange(config.exchange, 'topic', durable=True)
            for topic in config.rt_topics:
                self.topics_to_instances[topic].append(instance)
                queue = kombu.Queue(exchange=exchange, durable=True,
                                    routing_key=topic)
                self.queues.append(queue)

    def run(self):
        self._init()
        super(AtReloader, self).run()

    def get_consumers(self, Consumer, channel):
        return [Consumer(queues=self.queues, callbacks=[self.handle_task])]

    def handle_task(self, body, task):
        topic = task.delivery_info['routing_key']
        for instance in self.topics_to_instances[topic]:
            if self.last_reload[instance.id] + timedelta(seconds=10) \
                    < datetime.now():
                logger.info('launch reload AT on {}'.format(instance.name))
                self.last_reload[instance.id] = datetime.now()
                #we wait 10 second before loading at
                #this way we don't have to load for each messages
                reload_at.apply_async(args=[instance.id],  countdown=10)

    def __del__(self):
        self.close()

    def close(self):
        if self.connection and self.connection.connected:
            self.connection.release()








