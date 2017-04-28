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

import glob
import logging
import os
import shutil
import re
import zipfile

from celery import chain
from celery.signals import task_postrun
from flask import current_app
import kombu

from tyr.binarisation import gtfs2ed, osm2ed, ed2nav, fusio2ed, geopal2ed, fare2ed, poi2ed, synonym2ed, \
    shape2ed, load_bounding_shape, bano2mimir, osm2mimir, stops2mimir
from tyr.binarisation import reload_data, move_to_backupdirectory
from tyr import celery
from navitiacommon import models, task_pb2, utils
from tyr.helper import load_instance_config, get_instance_logger
from navitiacommon.launch_exec import launch_exec


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
        'shape': shape2ed,
    }

    for _file in files:
        filename = None

        dataset = models.DataSet()
        # NOTE: for the moment we do not use the path to load the data here
        # but we'll need to refactor this to take it into account
        dataset.type, _ = utils.type_of_data(_file)
        dataset.family_type = utils.family_of_data(dataset.type)
        if dataset.type in task:
            if backup_file:
                filename = move_to_backupdirectory(_file,
                                                   instance_config.backup_directory)
            else:
                filename = _file
            actions.append(task[dataset.type].si(instance_config, filename, dataset_uid=dataset.uid))
        else:
            #unknown type, we skip it
            current_app.logger.debug("unknown file type: {} for file {}"
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
        #We pass the job id to each tasks, but job need to be commited for having an id
        binarisation = [ed2nav.si(instance_config, job.id, custom_output_dir)]
        actions.append(chain(*binarisation))
        if dataset.family_type == 'pt' and instance.import_stops_in_mimir:
            # if we are loading pt data we might want to load the stops to autocomplete
            actions.append(stops2mimir.si(instance_config, filename, job.id, dataset_uid=dataset.uid))
        if reload:
            actions.append(reload_data.si(instance_config, job.id))
        actions.append(finish_job.si(job.id))
        if async:
            return chain(*actions).delay()
        else:
            # all job are run in sequence and import_data will only return when all the jobs are finish
            return chain(*actions).apply()


@celery.task()
def update_data():
    for instance in models.Instance.query_existing().all():
        current_app.logger.debug("Update data of : {}".format(instance.name))
        instance_config = load_instance_config(instance.name)
        files = glob.glob(instance_config.source_directory + "/*")
        import_data(files, instance, backup_file=True)


BANO_REGEXP = re.compile('.*bano.*')


def type_of_autocomplete_data(filename):
    """
    return the type of autocomplete data of the files
    filename can be either a directory, a file or a list of files

    return can be:
        - 'bano'
        - 'osm'

    """
    def files_type(files):
        #first we try fusio, because it can load fares too
        if any(f for f in files if BANO_REGEXP.match(f)):
            return 'bano'
        if len(files) == 1 and files[0].endswith('.pbf'):
            return 'osm'
        return None

    if not isinstance(filename, list):
        if filename.endswith('.zip'):
            zipf = zipfile.ZipFile(filename)
            files = zipf.namelist()
        elif os.path.isdir(filename):
            files = glob.glob(filename + "/*")
        else:
            files = [filename]
    else:
        files = filename

    return files_type(files)


@celery.task()
def import_autocomplete(files, autocomplete_instance, async=True, backup_file=True):
    """
    Import the autocomplete'instance data files
    """
    job = models.Job()
    actions = []

    task = {
        'bano': bano2mimir,
        'osm': osm2mimir,
    }
    autocomplete_dir = current_app.config['TYR_AUTOCOMPLETE_DIR']

    for _file in files:
        dataset = models.DataSet()
        dataset.type = type_of_autocomplete_data(_file)
        dataset.family_type = 'autocomplete_{}'.format(dataset.type)
        if dataset.type in task:
            if backup_file:
                filename = move_to_backupdirectory(_file, autocomplete_instance.backup_dir(autocomplete_dir))
            else:
                filename = _file
            actions.append(task[dataset.type].si(autocomplete_instance,
                                                 filename=filename, dataset_uid=dataset.uid))
        else:
            #unknown type, we skip it
            current_app.logger.debug("unknown file type: {} for file {}"
                                     .format(dataset.type, _file))
            continue

        #currently the name of a dataset is the path to it
        dataset.name = filename
        models.db.session.add(dataset)
        job.data_sets.append(dataset)
        job.autocomplete_params_id = autocomplete_instance.id

    if not actions:
        return

    models.db.session.add(job)
    models.db.session.commit()
    for action in actions:
        action.kwargs['job_id'] = job.id
    actions.append(finish_job.si(job.id))
    if async:
        return chain(*actions).delay()
    else:
        # all job are run in sequence and import_data will only return when all the jobs are finish
        return chain(*actions).apply()


@celery.task()
def import_in_mimir(_file, instance, async=True):
    """
    Import pt data stops to autocomplete
    """
    current_app.logger.debug("Import pt data to mimir")
    instance_config = load_instance_config(instance.name)
    action = stops2mimir.si(instance_config, _file)
    if async:
        return action.delay()
    else:
        # all job are run in sequence and import_in_mimir will only return when all the jobs are finish
        return action.apply()


@celery.task()
def update_autocomplete():
    current_app.logger.debug("Update autocomplete data")
    autocomplete_dir = current_app.config['TYR_AUTOCOMPLETE_DIR']
    for autocomplete_instance in models.AutocompleteParameter.query.all():
        files = glob.glob(autocomplete_instance.source_dir(autocomplete_dir) + "/*")
        if files:
            import_autocomplete(files, autocomplete_instance, backup_file=True)


@celery.task()
def purge_datasets():
    instances = models.Instance.query_existing().all()
    for instance in instances:
        purge_instance(instance.id, current_app.config['DATASET_MAX_BACKUPS_TO_KEEP'])


@celery.task()
def purge_instance(instance_id, nb_to_keep):
    instance = models.Instance.query.get(instance_id)
    logger = get_instance_logger(instance)
    logger.info('purge of backup directories for %s', instance.name)
    instance_config = load_instance_config(instance.name)
    backups = set(glob.glob('{}/*'.format(instance_config.backup_directory)))
    logger.info('backups are: %s', backups)
    # we add the realpath not to have problems with double / or stuff like that
    loaded = set(os.path.realpath(os.path.dirname(dataset.name))
                 for dataset in instance.last_datasets(nb_to_keep))
    logger.info('loaded  data are: %s', loaded)
    to_remove = [os.path.join(instance_config.backup_directory, f) for f in backups - loaded]

    missing = [l for l in loaded if l not in backups]
    if missing:
        logger.error("MISSING backup files! impossible to find %s in the backup dir, "
                     "we skip the purge, repair ASAP to fix the purge", missing)
        return

    logger.info('we remove: %s', to_remove)
    for path in to_remove:
        shutil.rmtree(path)


@celery.task()
def scan_instances():
    for instance_file in glob.glob(current_app.config['INSTANCES_DIR'] + '/*.ini'):
        instance_name = os.path.basename(instance_file).replace('.ini', '')
        instance = models.Instance.query_existing().filter_by(name=instance_name).first()
        if not instance:
            current_app.logger.info('new instances detected: %s', instance_name)
            instance = models.Instance(name=instance_name)
            instance_config = load_instance_config(instance.name)
            instance.is_free = instance_config.is_free
            #by default we will consider an free instance as an opendata one
            instance.is_open_data = instance_config.is_free

            models.db.session.add(instance)
            models.db.session.commit()


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
    for instance in models.Instance.query_existing().all():
        build_data(instance)


@celery.task()
def build_data(instance):
    job = models.Job()
    job.instance = instance
    job.state = 'pending'
    instance_config = load_instance_config(instance.name)
    models.db.session.add(job)
    models.db.session.commit()
    chain(ed2nav.si(instance_config, job.id, None), finish_job.si(job.id)).delay()
    current_app.logger.info("Job build data of : %s queued" % instance.name)


@celery.task()
def load_data(instance_id, data_dirs):
    instance = models.Instance.query.get(instance_id)

    import_data(data_dirs, instance, backup_file=False, async=False)


@celery.task()
def cities(osm_path):
    """ launch cities """
    res = -1
    try:
        res = launch_exec("cities", ['-i', osm_path,
                                     '--connection-string',
                                     current_app.config['CITIES_DATABASE_URI']],
                          logging)
        if res != 0:
            logging.error('cities failed')
    except:
        logging.exception('')
    logging.info('Import of cities finished')
    return res


@celery.task()
def bounding_shape(instance_name, shape_path):
    """ Set the bounding shape to a custom value """

    instance_conf = load_instance_config(instance_name)

    load_bounding_shape(instance_name, instance_conf, shape_path)


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
        instances = models.Instance.query_existing().all()
        task = task_pb2.Task()
        task.action = task_pb2.HEARTBEAT

        for instance in instances:
            config = load_instance_config(instance.name)
            exchange = kombu.Exchange(config.exchange, 'topic', durable=True)
            producer = connection.Producer(exchange=exchange)
            producer.publish(task.SerializeToString(), routing_key='{}.task.heartbeat'.format(instance.name))


@celery.task()
def create_autocomplete_depot(name):
    autocomplete_dir = current_app.config['TYR_AUTOCOMPLETE_DIR']
    autocomplete = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
    main_dir = autocomplete.main_dir(autocomplete_dir)
    try:
        if not os.path.exists(main_dir):
            os.makedirs(main_dir)
        source = autocomplete.source_dir(autocomplete_dir)
        if not os.path.exists(source):
            os.makedirs(source)
        backup = autocomplete.backup_dir(autocomplete_dir)
        if not os.path.exists(backup):
            os.makedirs(backup)
    except OSError:
        logging.error('create directory {} failed'.format(main_dir))


@celery.task()
def remove_autocomplete_depot(name):
    logging.info('removing instance dir for {}'.format(name))
    autocomplete_dir = current_app.config['TYR_AUTOCOMPLETE_DIR']
    if os.path.exists(autocomplete_dir):
        autocomplete = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        main_dir = autocomplete.main_dir(autocomplete_dir)
        if os.path.exists(main_dir):
            shutil.rmtree(main_dir)
        else:
            logging.warn('no autocomplete directory for {}, removing nothing'.format(autocomplete_dir))
    else:
        logging.warn('no main autocomplete directory, removing nothing')

@celery.task()
def purge_autocomplete():
    logger = logging.getLogger(__name__)
    autocomplete_instances = models.AutocompleteParameter.query.all()
    for ac_instance in autocomplete_instances:
        logger.info('purging autocomplete backup directories for %s', ac_instance.name)
        max_backups = current_app.config.get('AUOTOCOMPLETE_MAX_BACKUPS_TO_KEEP', 5)
        dir_to_keep = set(os.path.realpath(os.path.dirname(dataset.name))
                          for dataset in ac_instance.last_datasets(max_backups))
        autocomplete_dir = current_app.config['TYR_AUTOCOMPLETE_DIR']
        backup_dir = os.path.join(autocomplete_dir, ac_instance.name, 'backup')
        all_backups = set(os.path.join(backup_dir, backup)
                          for backup in os.listdir(backup_dir))
        to_remove = all_backups - dir_to_keep
        for directory in to_remove:
            if os.path.exists(directory):
                try:
                    logger.info('removing backup directory: %s', directory)
                    shutil.rmtree(directory)
                except Exception as e:
                    logger.info('cannot purge directory: %s because: %s', directory, str(e))
