#encoding: utf-8
from persistor.config import Config
import pika
import logging

def callback(ch, method, properties, body):
    logging.debug("Message received")
    ch.basic_ack(delivery_tag = method.delivery_tag)

class Persistor(object):
    def __init__(self):
        self.connection = None
        self.channel = None
        self.__init_logger()
        self.config = Config()

    def load_config(self, filename):
        self.config.load(filename)


    def run(self):
        self.__init_rabbitmq()
        logging.info("start consuming")
        self.channel.start_consuming()

    def __init_logger(self, filename='', level='debug'):
        level = getattr(logging, level.upper(), logging.DEBUG)
        logging.basicConfig(filename=filename, level=level)

    def __init_rabbitmq(self):
        """
        initialise les queue rabbitmq
        """
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(
            host=self.config.rabbitmq_host,
            port=self.config.rabbitmq_port,
            virtual_host=self.config.rabbitmq_vhost,
            credentials=pika.credentials.PlainCredentials(
                self.config.rabbitmq_username, self.config.rabbitmq_password)
            ))
        self.channel = self.connection.channel()
        instance_name = self.config.instance_name
        exchange_name = self.config.exchange_name
        self.channel.exchange_declare(exchange=exchange_name, type='topic',
                durable=True)
        #la queue pour persistor doit etre persistante
        #on veux pouvoir gérer la reprise sur incident
        queue_name = instance_name + '_persistor'
        self.channel.queue_declare(queue=queue_name, durable=True)
        logging.debug("listen following topics: %s", self.config.rt_topics)
        #on bind notre queue pour les différent topics spécifiés
        for binding_key in self.config.rt_topics:
            self.channel.queue_bind(exchange=exchange_name,
                    queue=queue_name, routing_key=binding_key)


        self.channel.basic_consume(callback, queue=queue_name)



    def __del__(self):
        self.close()

    def close(self):
        if self.connection:
            self.connection.close()




