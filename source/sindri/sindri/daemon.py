#encoding: utf-8
from sindri.config import Config
import pika
import logging
import sindri.task_pb2
import google
from sindri.saver import EdRealtimeSaver, TechnicalError, FunctionalError
import sys

class Sindri(object):
    """
    Classe gérant le service de persitances des événements temps réel envoyés
    sur rabbitmq.
    Sindri et son frére Brokk sont les nains forgerons de la mythologie nordique

    l'utilisation est la suivante:
    sindri = Sindri()
    sindri.init(conf_filename)
    sindri.run()
    """
    def __init__(self):
        self.connection = None
        self.channel = None
        self.ed_realtime_saver = None
        self.__init_logger()
        self.config = Config()

    def init(self, filename):
        """
        initialise le service via le fichier de conf passer en paramétre
        """
        self.config.load(filename)
#la DB doit etre prete avant d'initialiser rabbitmq, on peut recevoir des
#tache avant d'avoir lancer la boucle d'evenement
        self.ed_realtime_saver = EdRealtimeSaver(self.config)
        self.__init_rabbitmq()



    def run(self):
        """
        lance la boucle de traitement
        """
        logging.getLogger('sindri').info("start consuming")
        self.channel.start_consuming()

    def __init_logger(self, filename='', level='debug'):
        """
        initialise le logger, par défaut level=Debug
        et affichage sur la sortie standard
        """
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
        #la queue pour sindri doit etre persistante
        #on veux pouvoir gérer la reprise sur incident
        queue_name = instance_name + '_sindri'
        self.channel.queue_declare(queue=queue_name, durable=True)
        logging.getLogger('sindri').info("listen following topics: %s",
                self.config.rt_topics)
        #on bind notre queue pour les différent topics spécifiés
        for binding_key in self.config.rt_topics:
            self.channel.queue_bind(exchange=exchange_name,
                    queue=queue_name, routing_key=binding_key)


        self.channel.basic_consume(self.callback, queue=queue_name)

    def handle_message(self, task):
        if(task.message.IsInitialized()):
            try:
                self.ed_realtime_saver.persist_message(task.message)
            except FunctionalError, e:
                logging.getLogger('sindri').warn("%s", str(e))
        else:
            logging.getLogger('sindri').warn("message task whitout"
                    "message in payload")


    def callback(self, ch, method, properties, body):
        logging.getLogger('sindri').debug("Message received")
        task = sindri.task_pb2.Task()
        try:
            task.ParseFromString(body)
            logging.getLogger('sindri').debug('%s', str(task))
        except google.protobuf.message.DecodeError, e:
            logging.getLogger('sindri').warn("message is not a valid "
                    "protobuf task: %s", str(e))
            ch.basic_ack(delivery_tag = method.delivery_tag)
            return

        try:
            if(task.action == sindri.task_pb2.MESSAGE):
                self.handle_message(task)

            ch.basic_ack(delivery_tag = method.delivery_tag)
        except TechnicalError:
            #en cas d'erreur technique (DB KO) on acknoledge pas la tache
            #et on attend 10sec
            ch.basic_nack(delivery_tag = method.delivery_tag)
            self.connection.sleep(10)
        except:
            logging.exception('fatal')
            sys.exit(1)



    def __del__(self):
        self.close()

    def close(self):
        if self.connection:
            self.connection.close()



