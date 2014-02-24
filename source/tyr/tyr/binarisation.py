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
from navitiacommon import models
import shutil
from tyr.helper import get_instance_logger


def move_to_backupdirectory(filename, working_directory):
    """ If there is no backup directory it creates one in
        {instance_directory}/backup/{name}
        The name of the backup directory is the time when it's created
        formatted as %Y%m%d-%H%M%S
    """
    now = datetime.datetime.now()
    working_directory += "/" + now.strftime("%Y%m%d-%H%M%S%f")
    os.mkdir(working_directory, 0755)
    destination = working_directory + '/' + os.path.basename(filename)
    shutil.move(filename, destination)
    return destination


def make_connection_string(instance_config):
    """ Make a connection string connection from the config """
    connection_string = 'host=' + instance_config.pg_host
    connection_string += ' user=' + instance_config.pg_username
    connection_string += ' dbname=' + instance_config.pg_dbname
    connection_string += ' password=' + instance_config.pg_password
    return connection_string


#TODO bind task
@celery.task()
def fusio2ed(instance_config, filename, job_id):
    """ Unzip fusio file and launch fusio2ed """

    job = models.Job.query.get(job_id)
    instance = job.instance
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        fusio2ed.retry(countdown=300, max_retries=10)

    logger = get_instance_logger(instance)
    try:
        working_directory = os.path.dirname(filename)

        zip_file = zipfile.ZipFile(filename)
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
        res = launch_exec("fusio2ed", params, logger)
        if res != 0:
            raise ValueError('fusio2ed failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise
    finally:
        lock.release()


@celery.task()
def gtfs2ed(instance_config, gtfs_filename,  job_id):
    """ Unzip gtfs file launch gtfs2ed """

    job = models.Job.query.get(job_id)
    instance = job.instance
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        gtfs2ed.retry(countdown=300, max_retries=10)

    logger = get_instance_logger(instance)
    try:
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
        res = launch_exec("gtfs2ed", params, logger)
        if res != 0:
            raise ValueError('gtfs2ed failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise
    finally:
        lock.release()


@celery.task()
def osm2ed(instance_config, osm_filename, job_id):
    """ launch osm2ed """

    job = models.Job.query.get(job_id)
    instance = job.instance
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        osm2ed.retry(countdown=300, max_retries=10)

    logger = get_instance_logger(instance)
    try:
        connection_string = make_connection_string(instance_config)
        res = launch_exec('osm2ed',
                ["-i", osm_filename, "--connection-string", connection_string],
                logger)
        if res != 0:
            #@TODO: exception
            raise ValueError('osm2ed failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise
    finally:
        lock.release()

@celery.task()
def geopal2ed(instance_config, filename, job_id):
    """ launch geopal2ed """

    job = models.Job.query.get(job_id)
    instance = job.instance
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        geopal2ed.retry(countdown=300, max_retries=10)

    logger = get_instance_logger(instance)
    try:
        working_directory = os.path.dirname(filename)

        zip_file = zipfile.ZipFile(filename)
        zip_file.extractall(path=working_directory)

        connection_string = make_connection_string(instance_config)
        res = launch_exec('geopal2ed',
                ["-i", working_directory, "--connection-string", connection_string],
                logger)
        if res != 0:
            #@TODO: exception
            raise ValueError('geopal2ed failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise
    finally:
        lock.release()


@celery.task()
def reload_data(instance_config, job_id):
    """ reload data on all kraken of this instance"""
    job = models.Job.query.get(job_id)
    instance = job.instance
    logger = get_instance_logger(instance)
    try:
        task = navitiacommon.task_pb2.Task()
        task.action = navitiacommon.task_pb2.RELOAD

        connection = kombu.Connection(current_app.config['CELERY_BROKER_URL'])
        exchange = kombu.Exchange(instance_config.exchange, 'topic',
                                  durable=True)
        producer = connection.Producer(exchange=exchange)

        logger.info("reload kraken")
        producer.publish(task.SerializeToString(),
                routing_key=instance.name + '.task.reload')
        connection.release()
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise


@celery.task()
def ed2nav(instance_config, job_id):
    """ Launch ed2nav"""
    job = models.Job.query.get(job_id)
    instance = job.instance
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        ed2nav.retry(countdown=300, max_retries=10)

    logger = get_instance_logger(instance)
    try:
        filename = instance_config.tmp_file
        connection_string = make_connection_string(instance_config)
        res = launch_exec('ed2nav',
                    ["-o", filename, "--connection-string", connection_string],
                    logger)
        if res != 0:
            raise ValueError('ed2nav failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise
    finally:
        lock.release()


@celery.task()
def nav2rt(instance_config, job_id):
    """ Launch nav2rt"""
    job = models.Job.query.get(job_id)
    instance = job.instance
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        nav2rt.retry(countdown=300, max_retries=10)

    logger = get_instance_logger(instance)
    try:
        source_filename = instance_config.tmp_file
        target_filename = instance_config.target_file
        connection_string = make_connection_string(instance_config)
        res = launch_exec('nav2rt',
                    ["-i", source_filename, "-o", target_filename,
                        "--connection-string", connection_string],
                    logger)
        if res != 0:
            raise ValueError('nav2rt failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise
    finally:
        lock.release()


@celery.task()
def fare2ed(instance_config, filename, job_id):
    """ launch fare2ed """

    job = models.Job.query.get(job_id)
    instance = job.instance
    lock = redis.lock('tyr.lock|' + instance.name)
    if not lock.acquire(blocking=False):
        fare2ed.retry(countdown=300, max_retries=10)

    logger = get_instance_logger(instance)
    try:
        working_directory = os.path.dirname(filename)

        zip_file = zipfile.ZipFile(filename)
        zip_file.extractall(path=working_directory)

        res = launch_exec("fare2ed", ['-f', working_directory,
                                      '--connection-string',
                                      make_connection_string(instance_config)],
                          logger)
        if res != 0:
            #@TODO: exception
            raise ValueError('fare2ed failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise
    finally:
        lock.release()
