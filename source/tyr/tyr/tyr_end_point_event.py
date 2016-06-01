# encoding=utf-8

from tyr_rabbit_mq_handler import TyrRabbitMqHandler
from flask import current_app

class EndPointEventMessage(object):

    CREATE = 'create_end_point'
    UPDATE = 'update_end_point'
    DELETE = 'delete_end_point'

    def __init__(self, event_name, end_point, old_end_point_name=None):
        self._event_name = event_name
        self._end_point_old_name = old_end_point_name
        self._end_point = end_point

    def get_message(self):
        payload = {
            'event': self._event_name,
            'data': {
                'name': self._end_point.name,
                'default': self._end_point.default,
                'hostnames': self._end_point.hostnames
            }
        }

        if self._event_name == self.UPDATE:
            payload['data']['last_name'] = self._end_point_old_name

        return payload


class TyrEventsRabbitMq(object):

    def __init__(self):
        self._rabbit_mq_handler = TyrRabbitMqHandler()

    def request(self, end_point_event_message):
        self._rabbit_mq_handler.publish(end_point_event_message.get_message(), None, 'json')
