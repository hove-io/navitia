#coding=utf-8
"""
Class with the main loop
"""
import glob
from pyed.binarisation import osm2ed, fusio2ed, ed2nav, nav2rt
from datetime import datetime
from pyed.launch_exec import launch_exec
from pyed.config import ConfigException
import logging
import time
import pika

class Watching():
    """ when launch with run(), it looks in the source directory if there are
        any new file (osm or fusio) to compute.
        If there was any file it will launch ed2nav after
    """

    def __init__(self, conf):
        """ Init watching according to conf """
        self.directory = conf.get("instance", "source_directory")
        self.conf = conf
        self.pyed_logger = logging.getLogger('pyed')
        self.working_directory = None
        self.has_messages = False

    def make_backupdirectory(self):
        """ If there is no backup directory it creates one in
            {instance_directory}/backup/{name}
            The name of the backup directory is the time when it's created
            formatted as %Y%m%d-%H%M%S
        """
        if not self.working_directory:
            now = datetime.now()
            self.working_directory = self.conf.get("instance", "working_directory")
            self.working_directory += "/"+now.strftime("%Y%m%d-%H%M%S")
            return launch_exec("mkdir", [self.working_directory],
                               self.pyed_logger)

    def check_messages(self):
        """"""
        self.pyed_logger.debug("connection to rabbitmq")
        has_messages = False
        connection = pika.BlockingConnection(pika.ConnectionParameters(
            host=self.conf.get('broker', 'host'),
            port=int(self.conf.get('broker', 'port')),
            virtual_host=self.conf.get('broker', 'vhost'),
            credentials=pika.credentials.PlainCredentials(
                self.conf.get('broker', 'username'),
                self.conf.get('broker', 'password'))
            ))
        channel = connection.channel()
        instance_name = self.conf.get('instance', 'name')
        exchange_name = self.conf.get('broker', 'exchange')
        channel.exchange_declare(exchange=exchange_name, type='topic',
                durable=True)

        queue_name = instance_name + '_pyed'
        channel.queue_declare(queue=queue_name, durable=True)
        #on bind notre queue pour les différent topics spécifiés
        for binding_key in self.conf.get('broker', 'rt-topics'):
            channel.queue_bind(exchange=exchange_name,
                    queue=queue_name, routing_key=binding_key)

        self.pyed_logger.debug("start consumming: %s", queue_name)

        res = channel.basic_get(queue=queue_name, no_ack=True)
        if res[0]:
            has_messages = True
            #si on a des messages, on purge la queue
            channel.queue_purge(queue=queue_name)

        connection.close()
        self.pyed_logger.debug("rabbitmq connection closed")
        return has_messages

    def run(self):
        """ Every minutes it looks in the source directory if there is any file
            After it has computed the files it will launch ed2nav
        """
        while True:
            osm_files = glob.glob(self.directory+"/*.pbf")
            fusio_files = glob.glob(self.directory+"/*.zip")
            worked_on_files = []
            while len(osm_files) > 0 or len(fusio_files)>0 :
                try:
                    if self.make_backupdirectory() != 0:
                        self.pyed_logger.error("""Unable to make backup
                                                  directory""")
                except ConfigException:
                    self.pyed_logger.error("""Unable to make backup directory,
                                              conf error""")
                if len(osm_files) > 0:
                    osmfile = osm_files.pop()
                    self.pyed_logger.info("Osm file found " + osmfile )
                    res = osm2ed(osmfile, self.conf, self.working_directory)
                    if res == 0:
                        self.pyed_logger.info("Osm file add to ed " + osmfile )
                        worked_on_files.append(osmfile)
                    else:
                        self.pyed_logger.error("""Osm file's binarisation
                                                  failed %s"""% osmfile)
                elif len(fusio_files) > 0:
                    fusiofile = fusio_files.pop()
                    self.pyed_logger.info("fusio file found " + fusiofile )
                    res = fusio2ed(fusiofile, self.conf, self.working_directory)
                    if res == 0:
                        self.pyed_logger.info("""fusio file added
                                                 to ed %s""" % fusiofile )
                        worked_on_files.append(fusiofile)
                    else:
                        self.pyed_logger.error("""fusio file's binarisation
                                                  failed %s""" % fusiofile)
                else:
                    self.pyed_logger.debug("No fusio nor osm file found")
                osm_files = glob.glob(self.directory+"/*.pbf")
                fusio_files = glob.glob(self.directory+"/*.zip")

            if len(worked_on_files) > 0:
                self.pyed_logger.info("Launching ed2nav")
                res = ed2nav(self.conf)
                joined_files_str = ", ".join(worked_on_files)
                if res == 0:
                    self.pyed_logger.info("""Ed2nav has finished for the files
                                            : %s"""%joined_files_str)
                else:
                    self.pyed_logger.error("""Ed2nav failed for the files
                                             : %s"""%joined_files_str)


            else:
                self.pyed_logger.debug("We haven't made any binarisation")
            if worked_on_files or self.check_messages():
                nav2rt(self.conf)

            self.working_directory = None
            time.sleep(60)
