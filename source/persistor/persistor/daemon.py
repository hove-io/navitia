#encoding: utf-8
from persistor.config import Config
import pika
import logging
import persistor.task_pb2
import psycopg2
import google
from persistor.saver import EdRealtimeSaver, TechnicalError, InvalidMessage

class Persistor(object):
    def __init__(self):
        self.connection = None
        self.channel = None
        self.ed_realtime_saver = None
        self.__init_logger()
        self.config = Config()

    def init(self, filename):
        self.config.load(filename)
#la DB doit etre prete avant d'initialiser rabbitmq, on peut recevoir des
#tache avant d'avoir lancer la boucle d'evenement
        self.ed_realtime_saver = EdRealtimeSaver(self.config)
        self.__init_rabbitmq()



    def run(self):
        logging.getLogger('persistor').info("start consuming")
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
        logging.getLogger('persistor').debug("listen following topics: %s",
                self.config.rt_topics)
        #on bind notre queue pour les différent topics spécifiés
        for binding_key in self.config.rt_topics:
            self.channel.queue_bind(exchange=exchange_name,
                    queue=queue_name, routing_key=binding_key)


        self.channel.basic_consume(self.callback, queue=queue_name)

    def handle_message(self, task):
        logging.getLogger('persistor').debug('%s', str(task))
        if(task.message.IsInitialized()):
            try:
                self.ed_realtime_saver.persist_message(task.message)
            except InvalidMessage, e:
                logging.getLogger('persistor').warn("%s", str(e))
        else:
            logging.getLogger('persistor').warn("message task whitout"
                    "message in payload")


    def callback(self, ch, method, properties, body):
        logging.getLogger('persistor').debug("Message received")
        task = persistor.task_pb2.Task()
        try:
            task.ParseFromString(body)
        except google.protobuf.message.DecodeError, e:
            logging.getLogger('persistor').warn("message is not a valid "
                    "protobuf task: %s", str(e))
            ch.basic_ack(delivery_tag = method.delivery_tag)
            return

        if(task.action == persistor.task_pb2.MESSAGE):
            try:
                self.handle_message(task)
                ch.basic_ack(delivery_tag = method.delivery_tag)
            except TechnicalError:
                #en cas d'erreur technique (DB KO) on acknoledge pas la tache
                #et on attend 10sec
                ch.basic_nack(delivery_tag = method.delivery_tag)
                self.connection.sleep(10)


    def __del__(self):
        self.close()

    def close(self):
        if self.connection:
            self.connection.close()



