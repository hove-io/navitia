"""
Functions to launch the binaratisations
"""
from pyed.launch_exec import launch_exec
from pyed.config import ConfigException
import logging
import subprocess
import os
import pyed.task_pb2
import pika

def make_connection_string(config):
    """ Make the a connection string connection from the config """
    connection_string = 'host='+config.get("database", "host")
    connection_string += " user="+config.get("database", "user")
    connection_string += " dbname="+config.get("database", "name")
    connection_string += " password="+config.get("database", "password")
    return connection_string

def gtfs2ed(gtfs_filename, config, backup_directory):
    """ Unzip gtfs file, remove the file, launch gtfs2ed """
    pyed_logger = logging.getLogger('pyed')
    gtfs_logger = logging.getLogger('gtfs2ed')
    res = launch_exec("mv", [gtfs_filename, backup_directory], pyed_logger)
    if res != 0:
        return 1
    gtfs_bnanme = os.path.basename(gtfs_filename)
    new_gtfs = backup_directory + "/" +gtfs_bnanme
    res = launch_exec("unzip", [new_gtfs, "-d", backup_directory],
                       pyed_logger)
    if res != 0:
        return 2
    try :
        connection_string = make_connection_string(config)
    except ConfigException:
        pyed_logger.error("gtfs2ed : Unable to make the connection string")
        return 3
    res = launch_exec(config.get("instance", "exec_directory")+"/gtfs2ed",
                ["-i", backup_directory, "--connection-string",
                 connection_string],
                gtfs_logger, pyed_logger)
    if res != 0:
        return 4
    return 0


def osm2ed(osm_filename, config, backup_directory):
    """ Move osm file to backup directory, launch osm2ed """
    pyed_logger = logging.getLogger('pyed')
    osm_logger = logging.getLogger('osm2ed')
    res = launch_exec("mv", [osm_filename, backup_directory], pyed_logger)
    if res != 0:
        error = "Error while moving " + osm_filename
        error += + " to " + backup_directory
        osm_logger(error)
        pyed_logger(error)
        return 1
    connection_string = ""
    try:
        connection_string = make_connection_string(config)
    except ConfigException:
        error = "osm2ed : Unable to make the connection string"
        pyed_logger.error(error)
        osm_logger.error(error)
        return 2
    mved_file = backup_directory
    mved_file += "/"+osm_filename.split("/")[-1]
    res = launch_exec(config.get("instance", "exec_directory")+"/osm2ed",
                ["-i", mved_file, "--connection-string", connection_string],
                osm_logger, pyed_logger)
    if res != 0:
        error = "osm2ed failed"
        pyed_logger.error(error)
        osm_logger.error(error)
        return 3
    return 0


def reload_data(config):
    """ reload data on all kraken"""
    task = pyed.task_pb2.Task()
    task.action = pyed.task_pb2.RELOAD
    pyed_logger = logging.getLogger('pyed')

    #TODO configurer la connection!!!
    connection = pika.BlockingConnection(pika.ConnectionParameters(
        host=config.get('broker', 'host'),
        port=int(config.get('broker', 'port')),
        virtual_host=config.get('broker', 'vhost'),
        credentials=pika.credentials.PlainCredentials(
            config.get('broker', 'username'),
            config.get('broker', 'password'))
        ))
    channel = connection.channel()
    instance_name = config.get('instance', 'name')
    exchange_name = config.get('broker', 'exchange')
    channel.exchange_declare(exchange=exchange_name, type='topic',
            durable=True)

    pyed_logger.info("reload kraken")
    channel.basic_publish(exchange=exchange_name,
            routing_key=instance_name+'.task.reload',
            body=task.SerializeToString())
    connection.close()


def ed2nav(config):
    """ Launch osm2ed, compute the md5 sum of it, and save the md5 """
    pyed_logger = logging.getLogger('pyed')
    ed2nav_logger = logging.getLogger('ed2nav')
    target_directory = None
    try :
        target_directory = config.get("instance" , "target_directory")
    except ConfigException, error:
        ed2nav_logger(error)
        return 4
    filename = target_directory +"/data.nav.lz4"
    try:
        connection_string = make_connection_string(config)
    except ConfigException:
        pyed_logger.error("osm2ed : Unable to make the connection string")
        return 1
    res = launch_exec(config.get("instance", "exec_directory")+"/ed2nav",
                ["-o", filename, "--connection-string", connection_string],
                ed2nav_logger, pyed_logger)
    if res != 0:
        return 2
    reload_data(config)
    return 0
