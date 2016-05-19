# encoding=utf-8

from kombu import Exchange, Connection, Producer
import logging

from flask import current_app

class RabbitMqHandler(object):

    def __init__(self, exchange_name, type='direct', durable=True):
        try:
            self._connection = Connection(current_app.config['CELERY_BROKER_URL'])
            self._producer = Producer(self._connection)
            self._task_exchange = Exchange(name=exchange_name, type=type, durable=durable)
        except Exception:
            logging.exception('Unable to activate the producer')
            raise

    def errback(exc, interval):
        logging.info('Error: %r', exc, exc_info=1)
        logging.info('Retry in %s seconds.', interval)

    def publish(self, payload, routing_key=None, serializer='json'):
        publish = self._connection.ensure(self._producer, self._producer.publish, errback = self.errback, max_retries=3)
        publish(payload,
                serializer=serializer,
                exchange=self._task_exchange,
                declare=[self._task_exchange],
                routing_key=routing_key)

        logging.info("Sent: %s" % payload)
        self._connection.release()
