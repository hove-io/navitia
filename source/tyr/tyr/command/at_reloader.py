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








