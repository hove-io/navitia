# encoding: utf-8
from stat_persistor.config import Config
import logging
import navitiacommon.stat_pb2
import google
from stat_persistor.saver.statsaver import StatSaver
from stat_persistor.saver.utils import TechnicalError, FunctionalError
import sys
import time
import kombu
from kombu.mixins import ConsumerMixin


class StatPersistor(ConsumerMixin):

    """
    this is the service who handle the persistence of statistiques send to
    rabbitmq

    usage:
    sindri = Sindri()
    sindri.init(conf_filename)
    sindri.run()
    """

    def __init__(self):
        self.connection = None
        self.exchange = None
        self.stat_saver = None
        self.queues = []
        self.config = Config()

    def init(self, filename):
        """
        init the service with the configuration file taken in parameter
        """
        self.config.load(filename)
        self._init_logger(self.config.log_file, self.config.log_level)
        self.stat_saver = StatSaver(self.config)
        self._init_rabbitmq()


    def _init_logger(self, filename='', level='debug'):
        """
        initialise loggers, by default to debug level and with output on stdout
        """
        level = getattr(logging, level.upper(), logging.DEBUG)
        logging.basicConfig(filename=filename, level=level)

        if level == logging.DEBUG:
            # if we are in debug we log all sql request and results
            logging.getLogger('sqlalchemy.engine').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.pool').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.dialects.postgresql')\
                .setLevel(logging.INFO)

    def _init_rabbitmq(self):
        """
        connect to rabbitmq and init the queues
        """
        self.connection = kombu.Connection(self.config.broker_url)
        exchange_name = self.config.exchange_name
        exchange = kombu.Exchange(exchange_name, type="direct")
        logging.getLogger('stat_persistor').info("listen following exchange: %s",
                                         self.config.exchange_name)

        queue_name = self.config.queue_name
        queue = kombu.Queue(queue_name, exchange=exchange, durable=True)
        self.queues.append(queue)
    def get_consumers(self, Consumer, channel):
        return [Consumer(queues=self.queues, callbacks=[self.process_task])]

    def handle_statistique(self, stat_hit):
        if stat_hit.IsInitialized():
            try:
                self.stat_saver.persist_stat(self.config, stat_hit)
            except FunctionalError as e:
                logging.getLogger('stat_persistor').warn("%s", str(e))
            except TechnicalError as e:
                logging.getLogger('stat_persistor').warn("%s", str(e))

        else:
            logging.getLogger('stat_persistor').warn("stat task whitout "
                                             "payload")

    def process_task(self, body, message):
        logging.getLogger('stat_persistor').debug("Message received")
        stat_request = navitiacommon.stat_pb2.StatRequest()
        try:
            stat_request.ParseFromString(body)
            logging.getLogger('stat_persistor').debug('%s', str(stat_request))
        except google.protobuf.message.DecodeError as e:
            logging.getLogger('stat_persistor').warn("message is not a valid "
                                             "protobuf task: %s", str(e))
            message.ack()
            return

        try:
            self.handle_statistique(stat_request)

            message.ack()
        except TechnicalError:
            # on technical error (like a database KO) we retry this task later
            # and we sleep 10 seconds
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
