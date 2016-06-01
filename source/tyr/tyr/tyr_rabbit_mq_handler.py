# encoding=utf-8

from flask import current_app
from rabbit_mq_handler import RabbitMqHandler

class TyrRabbitMqHandler(RabbitMqHandler):

    def __init__(self):
        super(TyrRabbitMqHandler, self).__init__(current_app.config['CELERY_BROKER_URL'], "tyr_event_exchange")
