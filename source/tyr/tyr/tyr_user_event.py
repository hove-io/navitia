# encoding=utf-8

from tyr import tyr_rabbit_mq_handler
from flask import current_app


class RabbitMqMessage(object):
    def __init__(self):
        self._event_name = None
        self._billing_plan_name = None
        self._billing_plan_max_object_count = None
        self._billing_plan_max_request_count = None
        self._billing_plan_default = None
        self._end_point_name = None
        self._end_point_default = None
        self._login = None
        self._email = None
        self._block_until = None
        self._type = None
        self._last_login = None

    def get_message(self):
        return {
            'data': {
                'billing_plan': self.get_billing_plan(),
                'origin': self.get_end_point(),
                'username': self._login,
                'block_until': self._block_until,
                'email': self._email,
                'type': self._type,
            },
            'event': self._event_name,
        }

    def get_message_with_last_login(self):
        message = self.get_message()
        message['data']['last_username'] = self._last_login

        return message

    def get_end_point(self):
        return {'name': self._end_point_name, 'default': self._end_point_default}

    def get_billing_plan(self):
        return {
            'name': self._billing_plan_name,
            'max_object_count': self._billing_plan_max_object_count,
            'max_request_count': self._billing_plan_max_request_count,
            'max_default_count': self._billing_plan_default,
        }

    def set_event_name(self, event_name):
        self._event_name = event_name

    def set_last_login(self, last_login):
        self._last_login = last_login

    def set_user(self, user):
        self._email = user.email
        self._login = user.login
        if user.block_until:
            self._block_until = user.block_until.isoformat()
        self._type = user.type
        self._billing_plan_name = user.billing_plan.name
        self._billing_plan_default = user.billing_plan.default
        self._billing_plan_max_request_count = user.billing_plan.max_request_count
        self._billing_plan_max_object_count = user.billing_plan.max_object_count
        self._end_point_name = user.end_point.name
        self._end_point_default = user.end_point.default


class TyrUserEvent(object):
    def __init__(self):
        if not current_app.config['ENABLE_USER_EVENT']:
            return
        self._rabbit_mq_message = RabbitMqMessage()

    def request(self, user, event_name, last_login=None):
        if not current_app.config['ENABLE_USER_EVENT']:
            return
        self._rabbit_mq_message.set_user(user)
        self._rabbit_mq_message.set_event_name(event_name)

        if last_login is not None:
            self._rabbit_mq_message.set_last_login(last_login)
            tyr_rabbit_mq_handler.publish(self._rabbit_mq_message.get_message_with_last_login(), None, 'json')
        else:
            tyr_rabbit_mq_handler.publish(self._rabbit_mq_message.get_message(), None, 'json')
