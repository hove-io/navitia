# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

"""
Functions to launch the binarizations
"""

from __future__ import absolute_import, print_function, division
import logging
import os
import zipfile
import datetime
import shutil
from functools import wraps

from flask import current_app
from shapely.geometry import MultiPolygon
from shapely import wkt
from zipfile import BadZipfile
import sqlalchemy

from navitiacommon.launch_exec import launch_exec
import navitiacommon.task_pb2
from tyr import celery, redis
from tyr.rabbit_mq_handler import RabbitMqHandler
from navitiacommon import models
from tyr.helper import get_instance_logger, get_named_arg, get_autocomplete_instance_logger, get_task_logger
from contextlib import contextmanager
import glob
from redis.exceptions import ConnectionError
import retrying

from tyr.minio import MinioWrapper


def unzip_if_needed(filename):
    if not os.path.isdir(filename):
        # if it's not a directory, we consider it's a zip, and we unzip it
        working_directory = os.path.dirname(filename)
        try:
            zip_file = zipfile.ZipFile(filename)
            zip_file.extractall(path=working_directory)
        except BadZipfile:
            return filename  # the file is not a zip, we don't do anything
    else:
        working_directory = filename
    return working_directory


def zip_if_needed(filename):
    if os.path.isdir(filename):
        # if it's a directory, we zip it
        file = filename + ".zip"
        try:
            with zipfile.ZipFile(file, "w", zipfile.ZIP_DEFLATED) as zf:
                for dirname, _, files in os.walk(filename):
                    for _filename in files:
                        zf.write(os.path.join(dirname, _filename), _filename)
        except BadZipfile:
            return filename  # the file is a zip, we don't do anything
    else:
        file = filename
    return file


def manage_file_with_unwanted_char(file_basename, sub_string):
    """
    Returns a treated base file name if unwanted characters like '(', ')' are present
    :param file_basename: filename without full path
    :param sub_string: formatted string from datetime
    :return: treated base file name
    """
    unwanted_chars = ['(', ')', '{', '}']
    char_index = -1
    for char in unwanted_chars:
        char_index = file_basename.find(char)
        if char_index > -1:
            break
    if char_index > -1:
        file_ext = os.path.splitext(file_basename)[1]
        file_basename = '{}_{}{}'.format(file_basename[:char_index], sub_string, file_ext)
    return file_basename


def move_to_backupdirectory(filename, working_directory, manage_sp_char=False):
    """If there is no backup directory it creates one in
    {instance_directory}/backup/{name}
    The name of the backup directory is the time when it's created
    formatted as %Y%m%d-%H%M%S
    """
    now = datetime.datetime.now()
    sub_string = now.strftime("%Y%m%d-%H%M%S%f")
    working_directory += "/" + sub_string
    # this works even if the intermediate 'backup' dir does not exists
    os.makedirs(working_directory, 0o755)
    file_basename = os.path.basename(filename)
    # Rename destination filename as base_file_name + date_time if special characters are present
    if manage_sp_char:
        file_basename = manage_file_with_unwanted_char(file_basename, sub_string)

    destination = working_directory + '/' + file_basename
    shutil.move(filename, destination)
    return destination


def make_connection_string(instance_config):
    """Make a connection string connection from the config"""
    connection_string = 'host=' + instance_config.pg_host
    connection_string += ' user=' + instance_config.pg_username
    connection_string += ' dbname=' + instance_config.pg_dbname
    connection_string += ' password=' + instance_config.pg_password
    connection_string += ' port={port}'.format(port=instance_config.pg_port)
    return connection_string


def make_ed_common_params(instance_config, ed_action):
    common_params = list()
    common_params.extend(
        [
            "--connection-string",
            make_connection_string(instance_config),
            "--log_comment",
            '{}.{}'.format(instance_config.name, ed_action),
        ]
    )
    if current_app.config.get('USE_LOCAL_SYS_LOG'):
        common_params.append("--local_syslog")
    return common_params


def lock_release(lock, logger):
    token = None
    if hasattr(lock, 'local') and hasattr(lock.local, 'token'):
        # we store the token of the lock to be able to restore it later in case of error
        token = lock.local.token
    try:
        lock.release()
    except:
        if token:
            # release failed and token has been invalidated, any retry will fail, we restore the token
            # so in case of a connection error we will reconnect and release the lock
            lock.local.token = token
        logger.exception("exception when trying to release lock, will retry")
        raise


class Lock(object):
    def __init__(self, timeout):
        self.timeout = timeout

    def __call__(self, func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            job_id = get_named_arg('job_id', func, args, kwargs)
            logging.debug('args: %s -- kwargs: %s', args, kwargs)
            job = models.Job.query.get(job_id)
            logger = get_instance_logger(job.instance, task_id=job_id)
            task = args[func.func_code.co_varnames.index('self')]
            try:
                lock = redis.lock('tyr.lock|' + job.instance.name, timeout=self.timeout)
                locked = lock.acquire(blocking=False)
            except ConnectionError:
                logging.exception('Exception with redis while locking. Retrying in 10sec')
                task.retry(countdown=10, max_retries=10)
            if not locked:
                countdown = 300
                logger.info('lock on %s retry %s in %s sec', job.instance.name, func.__name__, countdown)
                task.retry(countdown=countdown, max_retries=10)
            else:
                try:
                    logger.debug('lock acquired on %s for %s', job.instance.name, func.__name__)
                    return func(*args, **kwargs)
                finally:
                    logger.debug('release lock on %s for %s', job.instance.name, func.__name__)
                    # sometimes we are disconnected from redis when we want to release the lock,
                    # so we retry only the release
                    try:
                        retrying.Retrying(stop_max_attempt_number=5, wait_fixed=1000).call(
                            lock_release, lock, logger
                        )
                    except ValueError:  # LockError(ValueError) since redis 3.0
                        logger.exception(
                            "impossible to release lock: continue but following task may be locked :("
                        )

        return wrapper


@contextmanager
def collect_metric(task_type, job, dataset_uid):
    begin = datetime.datetime.utcnow()
    yield
    end = datetime.datetime.utcnow()
    try:
        dataset = models.DataSet.find_by_uid(dataset_uid)
        metric = models.Metric()
        metric.job = job
        metric.dataset = dataset
        metric.type = task_type
        metric.duration = end - begin
        models.db.session.add(metric)
        models.db.session.commit()
    except:
        logger = logging.getLogger(__name__)
        logger.exception('unable to persist Metrics data: ')


def _retrieve_dataset_and_set_state(dataset_type, job_id):
    dataset = models.DataSet.find_by_type_and_job_id(dataset_type, job_id)
    logging.getLogger(__name__).debug("Retrieved dataset: {}".format(dataset.id))
    dataset.state = "running"
    models.db.session.commit()
    return dataset


@celery.task(bind=True)
@Lock(timeout=30 * 60)
def fusio2ed(self, instance_config, filename, job_id, dataset_uid):
    """Unzip fusio file and launch fusio2ed"""

    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state("fusio", job.id)
    instance = job.instance

    logger = get_instance_logger(instance, task_id=job_id)
    try:
        working_directory = unzip_if_needed(filename)

        params = ["-i", working_directory]
        if instance_config.aliases_file:
            params.append("-a")
            params.append(instance_config.aliases_file)

        if instance_config.synonyms_file:
            params.append("-s")
            params.append(instance_config.synonyms_file)

        common_params = make_ed_common_params(instance_config, 'fusio2ed')
        params.extend(common_params)

        res = None
        with collect_metric("fusio2ed", job, dataset_uid):
            res = launch_exec("fusio2ed", params, logger)
        if res != 0:
            raise ValueError("fusio2ed failed")
        dataset.state = "done"
    except:
        logger.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()


@celery.task(bind=True)
@Lock(30 * 60)
def gtfs2ed(self, instance_config, gtfs_filename, job_id, dataset_uid):
    """Unzip gtfs file launch gtfs2ed"""

    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state("gtfs", job.id)
    instance = job.instance

    logger = get_instance_logger(instance, task_id=job_id)
    try:
        working_directory = unzip_if_needed(gtfs_filename)

        params = ["-i", working_directory]
        if instance_config.aliases_file:
            params.append("-a")
            params.append(instance_config.aliases_file)

        if instance_config.synonyms_file:
            params.append("-s")
            params.append(instance_config.synonyms_file)

        common_params = make_ed_common_params(instance_config, 'gtfs2ed')
        params.extend(common_params)

        res = None
        with collect_metric("gtfs2ed", job, dataset_uid):
            res = launch_exec("gtfs2ed", params, logger)
        if res != 0:
            raise ValueError("gtfs2ed failed")
        dataset.state = "done"
    except:
        logger.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()


@celery.task(bind=True)
@Lock(timeout=30 * 60)
def osm2ed(self, instance_config, osm_filename, job_id, dataset_uid):
    """launch osm2ed"""
    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state("osm", job.id)
    instance = job.instance
    poi_types_json = None
    if instance.poi_type_json:
        poi_types_json = instance.poi_type_json.poi_types_json

    osm_filename = unzip_if_needed(osm_filename)
    if os.path.isdir(osm_filename):
        osm_filename = glob.glob('{}/*.pbf'.format(osm_filename))[0]

    logger = get_instance_logger(instance, task_id=job_id)
    try:

        res = None
        params = ["-i", osm_filename]
        if poi_types_json:
            params.append('-p')
            import json

            # poi_types_json is just unicode string... we use this trick to one-line the json content
            params.append(u'{}'.format(json.dumps(json.loads(poi_types_json))))

        common_params = make_ed_common_params(instance_config, 'osm2ed')
        params.extend(common_params)

        if instance.admins_from_cities_db:
            cities_db = current_app.config.get('CITIES_DATABASE_URI')
            if not cities_db:
                raise ValueError(
                    "impossible to use osm2ed with cities db since no cities database configuration has been set"
                )
            params.extend(["--cities-connection-string", cities_db])
        with collect_metric("osm2ed", job, dataset_uid):
            res = launch_exec("osm2ed", params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError("osm2ed failed")
        dataset.state = "done"
    except:
        logger.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()


@celery.task(bind=True)
@Lock(timeout=30 * 60)
def geopal2ed(self, instance_config, filename, job_id, dataset_uid):
    """launch geopal2ed"""

    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state("geopal", job.id)
    instance = job.instance
    logger = get_instance_logger(instance, task_id=job_id)
    try:
        working_directory = unzip_if_needed(filename)

        res = None
        params = ["-i", working_directory]

        common_params = make_ed_common_params(instance_config, 'geopal2ed')
        params.extend(common_params)

        with collect_metric('geopal2ed', job, dataset_uid):
            res = launch_exec('geopal2ed', params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError('geopal2ed failed')
        dataset.state = "done"
    except:
        logger.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()


@celery.task(bind=True)
@Lock(timeout=10 * 60)
def poi2ed(self, instance_config, filename, job_id, dataset_uid):
    """launch poi2ed"""

    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state("poi", job.id)
    instance = job.instance
    logger = get_instance_logger(instance, task_id=job_id)
    try:
        working_directory = unzip_if_needed(filename)

        res = None
        params = ["-i", working_directory]

        common_params = make_ed_common_params(instance_config, 'poi2ed')
        params.extend(common_params)

        with collect_metric("poi2ed", job, dataset_uid):
            res = launch_exec("poi2ed", params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError("poi2ed failed")
        dataset.state = "done"
    except:
        logger.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()


@celery.task(bind=True)
@Lock(timeout=10 * 60)
def synonym2ed(self, instance_config, filename, job_id, dataset_uid):
    """launch synonym2ed"""

    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state("synonym", job.id)
    instance = job.instance

    logger = get_instance_logger(instance, task_id=job_id)
    try:
        res = None
        params = ["-i", filename]

        common_params = make_ed_common_params(instance_config, 'synonym2ed')
        params.extend(common_params)

        with collect_metric('synonym2ed', job, dataset_uid):
            res = launch_exec('synonym2ed', params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError("synonym2ed failed")
        dataset.state = "done"
    except:
        logger.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()


# from http://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Python_Parsing
def parse_poly(lines):
    """Parse an Osmosis polygon filter file.

    Accept a sequence of lines from a polygon file, return a shapely.geometry.MultiPolygon object.

    http://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Format
    """
    in_ring = False
    coords = []

    for (index, line) in enumerate(lines):
        if index == 0:
            # first line is junk.
            continue

        elif index == 1:
            # second line is the first polygon ring.
            coords.append([[], []])
            ring = coords[-1][0]
            in_ring = True

        elif in_ring and line.strip() == 'END':
            # we are at the end of a ring, perhaps with more to come.
            in_ring = False

        elif in_ring:
            # we are in a ring and picking up new coordinates.
            ring.append(map(float, line.split()))

        elif not in_ring and line.strip() == 'END':
            # we are at the end of the whole polygon.
            break

        elif not in_ring and line.startswith('!'):
            # we are at the start of a polygon part hole.
            coords[-1][1].append([])
            ring = coords[-1][1][-1]
            in_ring = True

        elif not in_ring:
            # we are at the start of a polygon part.
            coords.append([[], []])
            ring = coords[-1][0]
            in_ring = True

    return MultiPolygon(coords)


def load_bounding_shape(instance_name, instance_conf, shape_path):
    logging.info("loading bounding shape for {} from = {}".format(instance_name, shape_path))

    if shape_path.endswith(".poly"):
        with open(shape_path, "r") as myfile:
            shape = parse_poly(myfile.readlines())
    elif shape_path.endswith(".wkt"):
        with open(shape_path, "r") as myfile:
            shape = wkt.loads(myfile.read())
    else:
        logging.error("bounding_shape: {} has an unknown extension.".format(shape_path))
        return
    if not shape.is_valid:
        raise ValueError("shape isn't valid")

    connection_string = "postgres://{u}:{pw}@{h}:{port}/{db}".format(
        u=instance_conf.pg_username,
        pw=instance_conf.pg_password,
        h=instance_conf.pg_host,
        db=instance_conf.pg_dbname,
        port=instance_conf.pg_port,
    )
    engine = sqlalchemy.create_engine(connection_string)
    try:
        # create the line if it does not exists
        engine.execute(
            """
        INSERT INTO navitia.parameters (shape)
        SELECT NULL WHERE NOT EXISTS (SELECT * FROM navitia.parameters)
        """
        ).close()
        # update the line, simplified to approx 100m
        engine.execute(
            """
        UPDATE navitia.parameters
        SET shape_computed = FALSE, shape = ST_Multi(ST_SimplifyPreserveTopology(ST_GeomFromText('{shape}'), 0.001))
        """.format(
                shape=wkt.dumps(shape)
            )
        ).close()
    finally:
        engine.dispose()


@celery.task(bind=True)
@Lock(timeout=10 * 60)
def shape2ed(self, instance_config, filename, job_id, dataset_uid):
    """load a street network shape into ed"""
    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state("shape", job.id)
    instance = job.instance
    logging.info("loading bounding shape for {} from = {}".format(instance.name, filename))
    try:
        load_bounding_shape(instance.name, instance_config, filename)
        dataset.state = "done"
    except:
        logging.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()


@celery.task(bind=True)
def reload_data(self, instance_config, job_id):
    """reload data on all kraken of this instance"""
    job = models.Job.query.get(job_id)
    instance = job.instance
    logging.info("Unqueuing job {}, reload data of instance {}".format(job.id, instance.name))
    logger = get_instance_logger(instance, task_id=job_id)
    try:
        task = navitiacommon.task_pb2.Task()
        task.action = navitiacommon.task_pb2.RELOAD

        rabbit_mq_handler = RabbitMqHandler(
            current_app.config['KRAKEN_BROKER_URL'], instance_config.exchange, "topic"
        )

        logger.info("reload kraken")
        rabbit_mq_handler.publish(task.SerializeToString(), instance.name + '.task.reload')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise


@celery.task(bind=True)
@Lock(10 * 60)
def ed2nav(self, instance_config, job_id, custom_output_dir):
    """Launch ed2nav"""
    job = models.Job.query.get(job_id)
    instance = job.instance

    logger = get_instance_logger(instance, task_id=job_id)
    try:
        output_file = instance_config.target_file

        if custom_output_dir:
            # we change the target_filename to store it in a subdir
            target_path = os.path.join(os.path.dirname(output_file), custom_output_dir)
            output_file = os.path.join(target_path, os.path.basename(output_file))
            if not os.path.exists(target_path):
                os.makedirs(target_path)

        params = ["-o", output_file]
        if 'CITIES_DATABASE_URI' in current_app.config and current_app.config['CITIES_DATABASE_URI']:
            params.extend(["--cities-connection-string", current_app.config['CITIES_DATABASE_URI']])
        if instance.full_sn_geometries:
            params.extend(['--full_street_network_geometries'])

        common_params = make_ed_common_params(instance_config, 'ed2nav')
        params.extend(common_params)

        res = None
        with collect_metric('ed2nav', job, None):
            res = launch_exec('ed2nav', params, logger)
            os.system('sync')  # we sync to be safe
        if res != 0:
            raise ValueError('ed2nav failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise


@celery.task(bind=True)
@Lock(timeout=10 * 60)
def fare2ed(self, instance_config, filename, job_id, dataset_uid):
    """launch fare2ed"""

    job = models.Job.query.get(job_id)
    instance = job.instance

    logger = get_instance_logger(instance, task_id=job_id)
    try:

        working_directory = unzip_if_needed(filename)
        params = ["-f", working_directory]

        common_params = make_ed_common_params(instance_config, 'fare2ed')
        params.extend(common_params)

        res = launch_exec("fare2ed", params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError('fare2ed failed')
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise


def get_bano2mimir_params(working_directory, autocomplete_instance, autocomplete_version=2, cosmogony_file=None):
    if autocomplete_version == 2:
        return [
            '-i',
            working_directory,
            '--connection-string',
            current_app.config['MIMIR_URL'],
            '--dataset',
            autocomplete_instance.name,
        ]
    params = [
        '-s',
        'elasticsearch.url=\'{}\''.format(current_app.config['MIMIR7_URL']),
        '-s',
        'container.dataset=\'{}\''.format(autocomplete_instance.name),
        '-c',
        current_app.config['MIMIR_CONFIG_DIR'],
        '-i',
        working_directory,
        '-m',
        current_app.config['MIMIR_PLATFORM_TAG'],
    ]
    if cosmogony_file is not None:
        params.extend(['-s', 'admins.cosmogony_file=\'{}\''.format(cosmogony_file)])
    params.extend(['run'])
    return params


@celery.task(bind=True)
def bano2mimir(self, autocomplete_instance, filename, job_id, dataset_uid, autocomplete_version):
    """launch bano2mimir"""
    executable = "bano2mimir" if autocomplete_version == 2 else "bano2mimir7"
    autocomplete_instance = models.db.session.merge(autocomplete_instance)  # reatache the object
    logger = get_autocomplete_instance_logger(autocomplete_instance, task_id=job_id)
    if autocomplete_instance.address != 'BANO':
        logger.warning(
            'no bano data will be loaded for instance {} because the address are read from {}'.format(
                autocomplete_instance.name, autocomplete_instance.address
            )
        )
        return
    job = models.Job.query.get(job_id)
    working_directory = unzip_if_needed(filename)
    cosmogony_file = models.DataSet.get_cosmogony_file_path()
    params = get_bano2mimir_params(
        working_directory, autocomplete_instance, autocomplete_version, cosmogony_file
    )
    try:
        res = launch_exec(executable, params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError('{} failed'.format(executable))
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise


def get_openaddresses2mimir_params(
    autocomplete_instance, working_directory, autocomplete_version=2, cosmogony_file=None
):
    if autocomplete_version == 2:
        return [
            '-i',
            working_directory,
            '--connection-string',
            current_app.config['MIMIR_URL'],
            '--dataset',
            autocomplete_instance.name,
        ]
    params = [
        '-s',
        'elasticsearch.url=\'{}\''.format(current_app.config['MIMIR7_URL']),
        '-s',
        'container.dataset=\'{}\''.format(autocomplete_instance.name),
        '-c',
        current_app.config['MIMIR_CONFIG_DIR'],
        '-i',
        working_directory,
        '-m',
        current_app.config['MIMIR_PLATFORM_TAG'],
    ]

    if cosmogony_file is not None:
        params.extend(['-s', 'admins.cosmogony_file=\'{}\''.format(cosmogony_file)])
    params.extend(['run'])
    return params


@celery.task(bind=True)
def openaddresses2mimir(self, autocomplete_instance, filename, job_id, dataset_uid, autocomplete_version):
    """launch openaddresses2mimir"""
    executable = "openaddresses2mimir" if autocomplete_version == 2 else "openaddresses2mimir7"
    autocomplete_instance = models.db.session.merge(autocomplete_instance)  # reatache the object
    logger = get_autocomplete_instance_logger(autocomplete_instance, task_id=job_id)
    if autocomplete_instance.address != 'OA':
        logger.warning(
            'no open addresses data will be loaded for instance {} because the address are read from {}'.format(
                autocomplete_instance.name, autocomplete_instance.address
            )
        )
        return

    job = models.Job.query.get(job_id)
    working_directory = unzip_if_needed(filename)
    cosmogony_file = models.DataSet.get_cosmogony_file_path()
    params = get_openaddresses2mimir_params(
        autocomplete_instance, working_directory, autocomplete_version, cosmogony_file
    )
    try:
        res = launch_exec(executable, params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError('{} failed'.format(executable))
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise


def get_osm2mimir_params(
    autocomplete_instance,
    data_filename,
    working_directory,
    custom_config,
    autocomplete_version=2,
    cosmogony_file=None,
):
    if autocomplete_version == 2:
        return [
            '-i',
            data_filename,
            '--connection-string',
            current_app.config['MIMIR_URL'],
            '-D',
            "{}/".format(working_directory),
            '-s',
            custom_config,
        ]
    config_file = (
        '{}_{}'.format(current_app.config['MIMIR_PLATFORM_TAG'], autocomplete_instance.name)
        if current_app.config['MIMIR_PLATFORM_TAG'] != 'default'
        else autocomplete_instance.name
    )
    params = [
        '-s',
        'elasticsearch.url=\'{}\''.format(current_app.config['MIMIR7_URL']),
        '-i',
        data_filename,
        '-c',
        current_app.config['MIMIR_CONFIG_DIR'],
        '-m',
        config_file,
    ]

    if cosmogony_file is not None:
        params.extend(['-s', 'admins.cosmogony_file=\'{}\''.format(cosmogony_file)])
    params.extend(['run'])
    return params


@celery.task(bind=True)
def osm2mimir(self, autocomplete_instance, filename, job_id, dataset_uid, autocomplete_version):
    """launch osm2mimir"""
    executable = "osm2mimir" if autocomplete_version == 2 else "osm2mimir7"
    autocomplete_instance = models.db.session.merge(autocomplete_instance)  # reatache the object
    logger = get_autocomplete_instance_logger(autocomplete_instance, task_id=job_id)
    logger.debug('running {} for {}'.format(executable, job_id))
    job = models.Job.query.get(job_id)
    data_filename = unzip_if_needed(filename)
    custom_config = "custom_config"
    working_directory = os.path.dirname(data_filename)
    custom_config_config_toml = '{}/{}.toml'.format(working_directory, custom_config)
    data = autocomplete_instance.config_toml.encode("utf-8")
    cosmogony_file = models.DataSet.get_cosmogony_file_path()
    with open(custom_config_config_toml, 'w') as f:
        f.write(data)
    params = get_osm2mimir_params(
        autocomplete_instance,
        data_filename,
        working_directory,
        custom_config,
        autocomplete_version,
        cosmogony_file,
    )
    try:
        res = launch_exec(executable, params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError('{} failed'.format(executable))
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise


def get_stops2mimir_params(instance_name, working_directory, autocomplete_version=2, cosmogony_file=None):
    if autocomplete_version == 2:
        return [
            '--input',
            os.path.join(working_directory, 'stops.txt'),
            '--connection-string',
            current_app.config['MIMIR_URL'],
            '--dataset',
            instance_name,
        ]
    params = [
        '-s',
        'elasticsearch.url=\'{}\''.format(current_app.config['MIMIR7_URL']),
        '-s',
        'container.dataset=\'{}\''.format(instance_name),
        '-i',
        os.path.join(working_directory, 'stops.txt'),
        '-c',
        current_app.config['MIMIR_CONFIG_DIR'],
    ]
    if cosmogony_file is not None:
        params.extend(['-s', 'admins.cosmogony_file=\'{}\''.format(cosmogony_file)])
    params.extend(['run'])
    return params


@celery.task(bind=True)
def stops2mimir(self, instance_name, input, autocomplete_version, job_id=None, dataset_uid=None):
    """
    launch stops2mimir

    Note: this is temporary, this will be done by tartare when tartare will be available
    """
    # We don't have job_id while doing a reimport of all instances with import_stops_in_mimir = true
    if job_id:
        job = models.Job.query.get(job_id)
        instance = job.instance
        logger = get_instance_logger(instance, task_id=job_id)
    else:
        logger = get_task_logger(logging.getLogger("autocomplete"))
    executable = "stops2mimir" if autocomplete_version == 2 else "stops2mimir7"

    logger.debug('running {} for Es{}'.format(executable, autocomplete_version))
    cosmogony_file = models.DataSet.get_cosmogony_file_path()
    argv = get_stops2mimir_params(instance_name, os.path.dirname(input), autocomplete_version, cosmogony_file)

    try:
        res = launch_exec(executable, argv, logger)
        if res != 0:
            # Do not raise error because it breaks celery tasks chain.
            # stops2mimir has to be non-blocking.
            # @TODO : Find a way to raise error without breaking celery tasks chain
            logger.error('{} failed'.format(executable))
            if job_id:
                job.state = 'failed'
                models.db.session.commit()
    except:
        logger.exception('')
        if job_id:
            job.state = 'failed'
            models.db.session.commit()

        raise


def get_ntfs2mimir_params(instance_name, working_directory, autocomplete_version=2, cosmogony_file=None):
    if autocomplete_version == 2:
        return [
            '--input',
            working_directory,
            '--connection-string',
            current_app.config['MIMIR_URL'],
            '--dataset',
            instance_name,
        ]
    params = [
        '-s',
        'elasticsearch.url=\'{}\''.format(current_app.config['MIMIR7_URL']),
        '-s',
        'container.dataset=\'{}\''.format(instance_name),
        '-i',
        working_directory,
        '-c',
        current_app.config['MIMIR_CONFIG_DIR'],
        '-m',
        current_app.config['MIMIR_PLATFORM_TAG'],
    ]
    if cosmogony_file is not None:
        params.extend(['-s', 'admins.cosmogony_file=\'{}\''.format(cosmogony_file)])
    params.extend(['run'])
    return params


@celery.task(bind=True)
def ntfs2mimir(self, instance_name, input, autocomplete_version, job_id=None, dataset_uid=None):
    """
    launch ntfs2mimir
    """
    # We don't have job_id while doing a reimport of all instances with import_stops_in_mimir = true
    if job_id:
        job = models.Job.query.get(job_id)
        instance = job.instance
        logger = get_instance_logger(instance, task_id=job_id)
    else:
        logger = get_task_logger(logging.getLogger("autocomplete"))
    executable = "ntfs2mimir" if autocomplete_version == 2 else "ntfs2mimir7"
    logger.debug('running {} for Es{}'.format(executable, autocomplete_version))

    working_directory = unzip_if_needed(input)
    cosmogony_file = models.DataSet.get_cosmogony_file_path()
    argv = get_ntfs2mimir_params(instance_name, working_directory, autocomplete_version, cosmogony_file)
    try:
        res = launch_exec(executable, argv, logger)
        if res != 0:
            # Do not raise error because it breaks celery tasks chain.
            # ntfs2mimir have to be non-blocking.
            # @TODO : Find a way to raise error without breaking celery tasks chain
            logger.error('{} failed'.format(executable))
            if job_id:
                job.state = 'failed'
                models.db.session.commit()
    except:
        logger.exception('')
        if job_id:
            job.state = 'failed'
            models.db.session.commit()

        raise


def get_cosmogony2mimir_params(cosmo_file, autocomplete_instance, autocomplete_version=2):
    if autocomplete_version == 2:
        return [
            '--input',
            cosmo_file,
            '--connection-string',
            current_app.config['MIMIR_URL'],
            '--dataset',
            autocomplete_instance.name,
            '--french-id-retrocompatibility',
        ]
    return [
        "-s",
        'elasticsearch.url=\'{}\''.format(current_app.config['MIMIR7_URL']),
        "-s",
        'container.dataset=\'{}\''.format(autocomplete_instance.name),
        "-c",
        current_app.config['MIMIR_CONFIG_DIR'],
        "-i",
        cosmo_file,
        "-m",
        current_app.config['MIMIR_PLATFORM_TAG'],
        "run",
    ]


@celery.task(bind=True)
def cosmogony2mimir(self, autocomplete_instance, filename, job_id, dataset_uid, autocomplete_version):
    executable = "cosmogony2mimir" if autocomplete_version == 2 else "cosmogony2mimir7"
    autocomplete_instance = models.db.session.merge(autocomplete_instance)  # reattach the object
    logger = get_autocomplete_instance_logger(autocomplete_instance, task_id=job_id)
    logger.debug('running {} for {}, version autocomplete {}'.format(executable, job_id, autocomplete_version))
    job = models.Job.query.get(job_id)
    cosmo_file = unzip_if_needed(filename)
    params = get_cosmogony2mimir_params(cosmo_file, autocomplete_instance, autocomplete_version)
    try:
        res = launch_exec(executable, params, logger)
        if res != 0:
            # @TODO: exception
            raise ValueError('{} failed'.format(executable))
    except:
        logger.exception('')
        job.state = 'failed'
        models.db.session.commit()
        raise


def get_poi2mimir_params(poi_file, dataset_name, autocomplete_version=2, cosmogony_file=None):
    if autocomplete_version == 2:
        return [
            '--input',
            poi_file,
            '--connection-string',
            current_app.config['MIMIR_URL'],
            '--dataset',
            dataset_name,
            '--private',
        ]
    params = [
        '-s',
        'elasticsearch.url=\'{}\''.format(current_app.config['MIMIR7_URL']),
        '-s',
        'container.dataset=\'{}\''.format(dataset_name),
        '-c',
        current_app.config['MIMIR_CONFIG_DIR'],
        '-i',
        poi_file,
        '-m',
        current_app.config['MIMIR_PLATFORM_TAG'],
    ]
    if cosmogony_file is not None:
        params.extend(['-s', 'admins.cosmogony_file=\'{}\''.format(cosmogony_file)])
    params.extend(['run'])
    return params


@celery.task(bind=True)
def poi2mimir(self, instance_name, input, autocomplete_version, job_id=None, dataset_uid=None):
    """launch poi2mimir"""
    dataset_name = 'priv.{}'.format(instance_name)  # We give the dataset a prefix to prevent
    #   collision with other datasets.
    job = None
    # We don't have job_id while doing a reimport of all instances with import_stops_in_mimir = true
    if job_id:
        job = models.Job.query.get(job_id)
        instance = job.instance
        logger = get_instance_logger(instance, task_id=job_id)
    else:
        logger = get_task_logger(logging.getLogger("autocomplete"))
        instance = models.Instance.query_existing().filter_by(name=instance_name).first()
    executable = "poi2mimir" if autocomplete_version == 2 else "poi2mimir7"
    logger.debug('running {} version autocomplete {}'.format(executable, autocomplete_version))
    cosmogony_file = models.DataSet.get_cosmogony_file_path()
    argv = get_poi2mimir_params(input, dataset_name, autocomplete_version, cosmogony_file)
    try:
        if job:
            with collect_metric(executable, job, dataset_uid):
                res = launch_exec(executable, argv, logger)
        else:
            res = launch_exec(executable, argv, logger)

        if res != 0:
            # Do not raise error because it breaks celery tasks chain.
            logger.error('{} failed'.format(executable))
            if job_id:
                job.state = 'failed'
                models.db.session.commit()
        else:
            instance.poi_dataset = dataset_name
            models.db.session.commit()
    except:
        logger.exception('')
        if job_id:
            job.state = 'failed'
            models.db.session.commit()

        raise


@celery.task(bind=True)
def fusio2s3(self, instance_config, filename, job_id, dataset_uid):
    """Zip fusio file and launch fusio2s3"""
    filename = enrich_ntfs_with_addresses(filename, job_id, dataset_uid)
    _inner_2s3(self, "fusio", instance_config, filename, job_id, dataset_uid)


def enrich_ntfs_with_addresses(filename, job_id, dataset_uid):
    """launch enrich-ntfs-with-addresses"""

    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state("fusio", job.id)
    instance = job.instance

    logger = get_instance_logger(instance, task_id=job_id)
    filename = zip_if_needed(filename)

    file_dir = os.path.dirname(filename)
    file_basename = os.path.basename(filename)
    output_dir = file_dir + "/enriched_with_addresses"
    os.makedirs(output_dir, 0o755)
    output = output_dir + "/" + file_basename

    try:
        params = [
            "--input",
            filename,
            "--output",
            output,
            "--bragi-url",
            current_app.config['BRAGI_URL'],
        ]

        res = None
        with collect_metric("enrich-ntfs-with-addresses", job, dataset_uid):
            res = launch_exec("enrich-ntfs-with-addresses", params, logger)
        if res != 0:
            raise ValueError("enrich-ntfs-with-addresses failed")
        dataset.state = "done"
    except:
        logger.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()

    return output


@celery.task(bind=True)
def gtfs2s3(self, instance_config, filename, job_id, dataset_uid):
    """Zip fusio file and launch gtfs2s3"""
    _inner_2s3(self, "gtfs", instance_config, filename, job_id, dataset_uid)


def _inner_2s3(self, dataset_type, instance_config, filename, job_id, dataset_uid):
    job = models.Job.query.get(job_id)
    dataset = _retrieve_dataset_and_set_state(dataset_type, job.id)
    instance = job.instance

    logger = get_instance_logger(instance, task_id=job_id)
    try:
        filename = zip_if_needed(filename)

        dt_now_str = datetime.datetime.now().strftime("%Y-%m-%d-%H:%M:%S")
        tags = {
            "coverage": instance_config.name,
            "datetime": dt_now_str,
            "data_type": dataset_type,
            "tyr_data_path": filename,
        }

        file_key = "{coverage}/{dataset_type}.zip".format(
            coverage=instance_config.name, dataset_type=dataset_type
        )

        minio_wrapper = MinioWrapper()
        with collect_metric("{dataset_type}2s3".format(dataset_type=dataset_type), job, dataset_uid):
            minio_wrapper.upload_file(filename, file_key, metadata=tags, content_type="application/zip")

        dataset.state = "done"
    except:
        logger.exception("")
        job.state = "failed"
        dataset.state = "failed"
        raise
    finally:
        models.db.session.commit()
