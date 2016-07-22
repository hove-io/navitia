# encoding=utf-8

from kombu import Exchange, Connection, Producer
import logging

class RabbitMqHandler(object):

    def __init__(self, connection, exchange_name, type='direct', durable=True):
        self._logger = logging.getLogger(__name__)
        try:
            self._connection = Connection(connection)
            self._producer = Producer(self._connection)
            self._task_exchange = Exchange(name=exchange_name, type=type, durable=durable)
        except Exception:
            self._logger.info('badly formated token %s', auth).exception('Unable to activate the producer')
            raise

    def errback(exc, interval):
        self._logger.info('Error: %r', exc, exc_info=1)
        self._logger.info('Retry in %s seconds.', interval)

    def publish(self, payload, routing_key=None, serializer=None):
        publish = self._connection.ensure(self._producer, self._producer.publish, errback = self.errback, max_retries=3)
        publish(payload,
                serializer=serializer,
                exchange=self._task_exchange,
                declare=[self._task_exchange],
                routing_key=routing_key)
