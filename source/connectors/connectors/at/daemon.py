#encoding: utf-8
from connectors.config import Config
from selector.atreader import AtRealtimeReader
import logging
import pika
import connectors.at.task_pb2
import connectors.redis_queue


class ConnectorAT(object):
    def __init__(self):
        self.connection = None
        self.channel = None
        self.at_realtime_reader = None
        self._init_logger()
        self.config = Config()
        self.redis_queue = None

    def init(self, filename):
        """
        initialise le service via le fichier de conf passer en paramétre
        """
        self.config.load(filename)
        self._init_redisqueue()
        self.at_realtime_reader = AtRealtimeReader(self.config, self.redis_queue)
        self._init_rabbitmq()

    def _init_logger(self, filename='', level='debug'):
        """
        initialise le logger, par défaut level=Debug
        et affichage sur la sortie standard
        """
        level = getattr(logging, level.upper(), logging.DEBUG)
        logging.basicConfig(filename=filename, level=level)

        if level == logging.DEBUG:
            #on active les logs de sqlalchemy si on est en debug:
            #log des requetes et des resultats
            logging.getLogger('sqlalchemy.engine').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.pool').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.dialects.postgresql') \
                .setLevel(logging.INFO)

    def _init_redisqueue(self):
        self.redis_queue = connectors.redis_queue.RedisQueue(self.config.redisqueque_host,
                                                     self.config.redisqueque_port,
                                                     self.config.redisqueque_db,
                                                     self.config.redisqueque_password)

    def _init_rabbitmq(self):
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
        exchange_name = self.config.exchange_name
        self.channel.exchange_declare(exchange=exchange_name, type='topic',
                                      durable=True)

    def run(self):
        self.at_realtime_reader.execute()
        logging.getLogger('connector').info("put message to following topics: "
                                            "%s", self.config.rt_topics)
        for message in self.at_realtime_reader.message_list:
            task = connectors.at.task_pb2.Task()
            task.action = connectors.at.task_pb2.MESSAGE
            task.message.MergeFrom(message)
            for routing_key in self.config.rt_topics:
                exchange_name = self.config.exchange_name
                self.channel.basic_publish(exchange=exchange_name,
                                           routing_key=routing_key,
                                           body=task.SerializeToString())
        for perturbation in self.at_realtime_reader.perturbation_list:
            task = connectors.at.task_pb2.Task()
            task.action = connectors.at.task_pb2.AT_PERTURBATION
            task.at_perturbation.MergeFrom(perturbation)
            for routing_key in self.config.rt_topics:
                exchange_name = self.config.exchange_name
                self.channel.basic_publish(exchange=exchange_name,
                                           routing_key=routing_key,
                                           body=task.SerializeToString())
