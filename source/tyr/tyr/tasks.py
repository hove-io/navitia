# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from celery import chain, group
from celery.signals import task_postrun
from tyr.binarisation import gtfs2ed, osm2ed, ed2nav, nav2rt, fusio2ed, geopal2ed, fare2ed, poi2ed, synonym2ed
from tyr.binarisation import reload_data, move_to_backupdirectory
from flask import current_app
import glob
from tyr import celery
from navitiacommon import models, task_pb2
import logging
import os
import zipfile
from tyr.helper import load_instance_config, get_instance_logger
import shutil
from tyr.launch_exec import launch_exec
import kombu
from shapely.geometry import MultiPolygon, Polygon
from shapely import wkt
import sqlalchemy


def type_of_data(filename):
    """
    return the type of data contains in a file
    this type can be one  in:
     - 'gtfs'
     - 'fusio'
     - 'osm'
    """
    if filename.endswith('.pbf'):
        return 'osm'
    if filename.endswith('.zip'):
        zipf = zipfile.ZipFile(filename)

        #first we try fusio, because it can load fares too
        if "contributors.txt" in zipf.namelist():
            return 'fusio'

        if 'fares.csv' in zipf.namelist():
            return 'fare'
        else:
            return 'gtfs'
    if filename.endswith('.geopal'):
        return 'geopal'
    if filename.endswith('.poi'):
        return 'poi'
    if filename.endswith("synonyms.txt"):
        return 'synonym'
    return None

def family_of_data(type):
    """
    return the family type of a data type
    by example "geopal" and "osm" are in the "streetnework" family
    """
    mapping = {
        'osm': 'streetnetwork', 'geopal': 'streetnetwork',
        'synonym': 'synonym',
        'poi': 'poi',
        'fusio': 'pt', 'gtfs': 'pt',
        'fare': 'fare'
    }
    if type in mapping:
        return mapping[type]
    else:
        return None


@celery.task()
def finish_job(job_id):
    """
    use for mark a job as done after all the required task has been executed
    """
    job = models.Job.query.get(job_id)
    job.state = 'done'
    models.db.session.commit()


def import_data(files, instance, backup_file, async=True, reload=True, custom_output_dir=None):
    """
    import the data contains in the list of 'files' in the 'instance'

    :param files: files to import
    :param instance: instance to receive the data
    :param backup_file: If True the files are moved to a backup directory, else they are not moved
    :param async: If True all jobs are run in background, else the jobs are run in sequence the function will only return when all of them are finish
    :param reload: If True kraken would be reload at the end of the treatment

    run the whole data import process:

    - data import in bdd (fusio2ed, gtfs2ed, poi2ed, ...)
    - export bdd to nav file
    - update the jormungandr db with the new data for the instance
    - reload the krakens
    """
    actions = []
    job = models.Job()
    instance_config = load_instance_config(instance.name)
    job.instance = instance
    job.state = 'pending'
    task = {
        'gtfs': gtfs2ed,
        'fusio': fusio2ed,
        'osm': osm2ed,
        'geopal': geopal2ed,
        'fare': fare2ed,
        'poi': poi2ed,
        'synonym': synonym2ed,
    }

    for _file in files:
        filename = None

        dataset = models.DataSet()
        dataset.type = type_of_data(_file)
        dataset.family_type = family_of_data(dataset.type)
        if dataset.type in task:
            if backup_file:
                filename = move_to_backupdirectory(_file,
                                                   instance_config.backup_directory)
            else:
                filename = _file
            actions.append(task[dataset.type].si(instance_config, filename))
        else:
            #unknown type, we skip it
            current_app.logger.debug("unknwn file type: {} for file {}"
                                     .format(dataset.type, _file))
            continue

        #currently the name of a dataset is the path to it
        dataset.name = filename
        models.db.session.add(dataset)
        job.data_sets.append(dataset)

    if actions:
        models.db.session.add(job)
        models.db.session.commit()
        for action in actions:
            action.kwargs['job_id'] = job.id
        #We pass the job id to each tasks, but job need to be commited for
        #having an id
        binarisation = [ed2nav.si(instance_config, job.id, custom_output_dir),
                        nav2rt.si(instance_config, job.id, custom_output_dir)]
        #We pass the job id to each tasks, but job need to be commited for
        #having an id
        actions.append(chain(*binarisation))
        if reload:
            actions.append(reload_data.si(instance_config, job.id))
        actions.append(finish_job.si(job.id))
        if async:
            chain(*actions).delay()
        else:
            # all job are run in sequence and import_data will only return when all the jobs are finish
            chain(*actions).apply()


@celery.task()
def update_data():
    for instance in models.Instance.query.all():
        current_app.logger.debug("Update data of : {}".format(instance.name))
        instance_config = load_instance_config(instance.name)
        files = glob.glob(instance_config.source_directory + "/*")
        import_data(files, instance, backup_file=True)

@celery.task()
def purge_instance(instance_id, nb_to_keep):
    instance = models.Instance.query.get(instance_id)
    logger = get_instance_logger(instance)
    logger.info('purge of backup directories for %s', instance.name)
    instance_config = load_instance_config(instance.name)
    backups = set(glob.glob('{}/*'.format(instance_config.backup_directory)))
    logger.debug('backups are: %s', backups)
    loaded = set(os.path.dirname(dataset.name) for dataset in instance.last_datasets(nb_to_keep))
    logger.debug('loaded  data are: %s', loaded)
    to_remove = [os.path.join(instance_config.backup_directory, f) for f in backups - loaded]
    logger.info('we remove: %s', to_remove)
    for path in to_remove:
        shutil.rmtree(path)




@celery.task()
def scan_instances():
    for instance_file in glob.glob(current_app.config['INSTANCES_DIR'] + '/*.ini'):
        instance_name = os.path.basename(instance_file).replace('.ini', '')
        instance = models.Instance.query.filter_by(name=instance_name).first()
        if not instance:
            current_app.logger.info('new instances detected: %s', instance_name)
            instance = models.Instance(name=instance_name)
            instance_config = load_instance_config(instance.name)
            instance.is_free = instance_config.is_free

            models.db.session.add(instance)
            models.db.session.commit()


@celery.task()
def reload_at(instance_id):
    instance = models.Instance.query.get(instance_id)
    job = models.Job()
    job.instance = instance
    job.state = 'pending'
    instance_config = load_instance_config(instance.name)
    models.db.session.add(job)
    models.db.session.commit()
    chain(nav2rt.si(instance_config, job.id),
          reload_data.si(instance_config, job.id),
          finish_job.si(job.id)
          ).delay()


@celery.task()
def reload_kraken(instance_id):
    instance = models.Instance.query.get(instance_id)
    job = models.Job()
    job.instance = instance
    job.state = 'pending'
    instance_config = load_instance_config(instance.name)
    models.db.session.add(job)
    models.db.session.commit()
    chain(reload_data.si(instance_config, job.id), finish_job.si(job.id)).delay()
    logging.info("Task reload kraken for instance {} queued".format(instance.name))


@celery.task()
def build_all_data():
    for instance in models.Instance.query.all():
        build_data(instance)


@celery.task()
def build_data(instance):
    job = models.Job()
    job.instance = instance
    job.state = 'pending'
    instance_config = load_instance_config(instance.name)
    models.db.session.add(job)
    models.db.session.commit()
    chain(ed2nav.si(instance_config, job.id),
                        nav2rt.si(instance_config, job.id), finish_job.si(job.id)).delay()
    current_app.logger.info("Job build data of : %s queued"%instance.name)


@celery.task()
def load_data(instance_id, data_path):
    instance = models.Instance.query.get(instance_id)
    files = glob.glob(data_path + "/*")

    import_data(files, instance, backup_file=False, async=False)


@celery.task()
def cities(osm_path):
    """ launch cities """
    res = -1
    try:
        res = launch_exec("cities", ['-i', osm_path,
                                      '--connection-string',
                                      current_app.config['CITIES_DATABASE_URI']],
                          logging)
        if res!=0:
            logging.error('cities failed')
    except:
        logging.exception('')
    logging.info('Import of cities finished')
    return res


# from http://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Python_Parsing
def parse_poly(lines):
    """ Parse an Osmosis polygon filter file.

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


@celery.task()
def bounding_shape(instance_name, shape_path):
    """ Set the bounding shape to a custom value """
    shape = None
    if shape_path.endswith(".poly"):
        with open (shape_path, "r") as myfile:
            shape = parse_poly(myfile.readlines())
    elif shape_path.endswith(".wkt"):
        with open (shape_path, "r") as myfile:
            shape = wkt.loads(myfile.read())
    else:
        logging.error("bounding_shape: {} has an unknown extension.".format(shape_path))
        return

    conf = load_instance_config(instance_name)
    connection_string = "postgres://{u}:{pw}@{h}/{db}"\
        .format(u=conf.pg_username, pw=conf.pg_password, h=conf.pg_host, db=conf.pg_dbname)
    engine = sqlalchemy.create_engine(connection_string)
    # create the line if it does not exists
    engine.execute("""
    INSERT INTO navitia.parameters (shape)
    SELECT NULL WHERE NOT EXISTS (SELECT * FROM navitia.parameters)
    """).close()
    # update the line, simplified to approx 100m
    engine.execute("""
    UPDATE navitia.parameters
    SET shape_computed = FALSE, shape = ST_Multi(ST_SimplifyPreserveTopology(ST_GeomFromText('{shape}'), 0.001))
    """.format(shape=wkt.dumps(shape))).close()


@task_postrun.connect
def close_session(*args, **kwargs):
    # Flask SQLAlchemy will automatically create new sessions for you from
    # a scoped session factory, given that we are maintaining the same app
    # context, this ensures tasks have a fresh session (e.g. session errors
    # won't propagate across tasks)
    models.db.session.remove()


@celery.task()
def heartbeat():
    """
    send a heartbeat to all kraken
    """
    logging.info('ping krakens!!')
    with kombu.Connection(current_app.config['CELERY_BROKER_URL']) as connection:
        instances = models.Instance.query.all()
        task = task_pb2.Task()
        task.action = task_pb2.HEARTBEAT

        for instance in instances:
            config = load_instance_config(instance.name)
            exchange = kombu.Exchange(config.exchange, 'topic', durable=True)
            producer = connection.Producer(exchange=exchange)
            producer.publish(task.SerializeToString(), routing_key='{}.task.heartbeat'.format(instance.name))

