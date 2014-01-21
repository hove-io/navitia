# encoding: utf-8
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
    Classe gérant le service de persitances des événements temps réel envoyés
    sur rabbitmq.
    Sindri et Brokk sont des fréres nains forgerons de la mythologie nordique

    l'utilisation est la suivante:
    sindri = Sindri()
    sindri.init(conf_filename)
    sindri.run()
    """

    def __init__(self):
        self.connection = None
        self.exchange = None
        self.queues = []
        self.ed_realtime_saver = None
        self._init_logger()
        self.config = Config()

    def init(self, filename):
        """
        initialise le service via le fichier de conf passer en paramétre
        """
        self.config.load(filename)
        self.ed_realtime_saver = EdRealtimeSaver(self.config)
        self._init_rabbitmq()


    def _init_logger(self, filename='', level='debug'):
        """
        initialise le logger, par défaut level=Debug
        et affichage sur la sortie standard
        """
        level = getattr(logging, level.upper(), logging.DEBUG)
        logging.basicConfig(filename=filename, level=level)

        if level == logging.DEBUG:
            # on active les logs de sqlalchemy si on est en debug:
            # log des requetes et des resultats
            logging.getLogger('sqlalchemy.engine').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.pool').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.dialects.postgresql')\
                .setLevel(logging.INFO)

    def _init_rabbitmq(self):
        """
        initialise les queue rabbitmq
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
            # en cas d'erreur technique (DB KO) on acknoledge pas la tache
            # et on attend 10sec
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
