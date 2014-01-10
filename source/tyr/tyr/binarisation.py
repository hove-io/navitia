"""
Functions to launch the binaratisations
"""
from tyr.launch_exec import launch_exec
import logging
import os
import navitiacommon.task_pb2
import zipfile
from tyr import celery, redis
import datetime
from flask import current_app
import kombu


def move_to_backupdirectory(filename, working_directory):
    """ If there is no backup directory it creates one in
        {instance_directory}/backup/{name}
        The name of the backup directory is the time when it's created
        formatted as %Y%m%d-%H%M%S
    """
    now = datetime.datetime.now()
    working_directory += "/" + now.strftime("%Y%m%d-%H%M%S%f")
    os.mkdir(working_directory)
    destination = working_directory + '/' + os.path.basename(filename)
    print filename
    print destination
    os.rename(filename, destination)
    return destination


def make_connection_string(instance_config):
    """ Make a connection string connection from the config """
    connection_string = 'host=' + instance_config.pg_host
    connection_string += ' user=' + instance_config.pg_username
    connection_string += ' dbname=' + instance_config.pg_dbname
    connection_string += ' password=' + instance_config.pg_password
    return connection_string

@celery.task()
def fusio2ed(instance, instance_config, filename):
    """ Unzip gtfs file, remove the file, launch gtfs2ed """

    lock = redis.lock('pyed.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        gtfs2ed.retry(countdown=300, max_retries=10)

    try:
        pyed_logger = logging.getLogger('pyed')
        fusio_logger = logging.getLogger('fusio2ed')
        bnanme = os.path.basename(filename)
        working_directory = os.path.dirname(filename)

        zip_file = zipfile.ZipFile(filename);
        zip_file.extractall(path=working_directory)

        params = ["-i", working_directory]
        if instance_config.aliases_file:
            params.append("-a")
            params.append(instance_config.aliases_file)

        if instance_config.synonyms_file:
            params.append("-s")
            params.append(instance_config.synonyms_file)

        connection_string = make_connection_string(instance_config)
        params.append("--connection-string")
        params.append(connection_string)
        res = launch_exec("fusio2ed", params, fusio_logger, pyed_logger)
        if res != 0:
            #@TODO: exception
            raise ValueError('todo: exception')
    finally:
        lock.release()


@celery.task()
def gtfs2ed(instance, instance_config, gtfs_filename):
    """ Unzip gtfs file, remove the file, launch gtfs2ed """

    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        gtfs2ed.retry(countdown=300, max_retries=10)

    try:
        tyr_logger = logging.getLogger('tyr')
        gtfs_logger = logging.getLogger('gtfs2ed')
        working_directory = os.path.dirname(gtfs_filename)

        zip_file = zipfile.ZipFile(gtfs_filename)
        zip_file.extractall(path=working_directory)

        params = ["-i", working_directory]
        if instance_config.aliases_file:
            params.append("-a")
            params.append(instance_config.aliases_file)

        if instance_config.synonyms_file:
            params.append("-s")
            params.append(instance_config.synonyms_file)

        connection_string = make_connection_string(instance_config)
        params.append("--connection-string")
        params.append(connection_string)
        res = launch_exec("gtfs2ed", params, gtfs_logger, tyr_logger)
        if res != 0:
            #@TODO: exception
            raise ValueError('todo: exception')
    finally:
        lock.release()


@celery.task()
def osm2ed(instance, instance_config, osm_filename):
    """ Move osm file to backup directory, launch osm2ed """

    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        osm2ed.retry(countdown=300, max_retries=10)

    try:
        tyr_logger = logging.getLogger('tyr')
        osm_logger = logging.getLogger('osm2ed')

        connection_string = make_connection_string(instance_config)
        res = launch_exec('osm2ed',
                ["-i", osm_filename, "--connection-string", connection_string],
                osm_logger, tyr_logger)
        if res != 0:
            #@TODO: exception
            raise ValueError('todo: exception')
    finally:
        lock.release()


@celery.task()
def reload_data(instance, instance_config):
    """ reload data on all kraken"""
    task = tyr.task_pb2.Task()
    task.action = tyr.task_pb2.RELOAD
    tyr_logger = logging.getLogger('tyr')

    connection = kombu.Connection(current_app.config['CELERY_BROKER_URL'])
    exchange = kombu.Exchange(instance_config.exchange, 'topic',
                              durable=True)
    producer = connection.Producer(exchange=exchange)

    tyr_logger.info("reload kraken")
    producer.publish(task.SerializeToString(),
            routing_key=instance.name + '.task.reload')
    connection.release()


@celery.task()
def ed2nav(instance, instance_config):
    """ Launch ed2nav"""
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        ed2nav.retry(countdown=300, max_retries=10)

    try:
        tyr_logger = logging.getLogger('tyr')
        ed2nav_logger = logging.getLogger('ed2nav')
        filename = instance_config.tmp_file
        connection_string = make_connection_string(instance_config)
        res = launch_exec('ed2nav',
                    ["-o", filename, "--connection-string", connection_string],
                    ed2nav_logger, tyr_logger)
        if res != 0:
            raise ValueError('todo: exception')
    finally:
        lock.release()


@celery.task()
def nav2rt(instance, instance_config):
    """ Launch nav2rt"""
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        nav2rt.retry(countdown=300, max_retries=10)

    try:
        tyr_logger = logging.getLogger('tyr')
        nav2rt_logger = logging.getLogger('nav2rt')
        source_filename = instance_config.tmp_file
        target_filename = instance_config.target_file
        connection_string = make_connection_string(instance_config)
        res = launch_exec('nav2rt',
                    ["-i", source_filename, "-o", target_filename,
                        "--connection-string", connection_string],
                    nav2rt_logger, tyr_logger)
        if res != 0:
            raise ValueError('todo: exception')
    finally:
        lock.release()
