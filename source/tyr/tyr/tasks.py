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

from tyr.binarisation import (
    gtfs2ed,
    osm2ed,
    ed2nav,
    fusio2ed,
    geopal2ed,
    fare2ed,
    poi2ed,
    synonym2ed,
    shape2ed,
    load_bounding_shape,
    bano2mimir,
    openaddresses2mimir,
    osm2mimir,
    stops2mimir,
    ntfs2mimir,
    cosmogony2mimir,
    poi2mimir,
    fusio2s3,
    gtfs2s3,
    zip_if_needed,
)
from tyr.binarisation import reload_data, move_to_backupdirectory
from tyr import celery
from navitiacommon import models, task_pb2, utils
from tyr.helper import (
    load_instance_config,
    get_instance_logger,
    is_activate_autocomplete_version,
    get_instances_name,
    create_autocomplete_instance_paths,
)
from navitiacommon.launch_exec import launch_exec
from datetime import datetime, timedelta


@celery.task()
def finish_job(job_id):
    """
    use for mark a job as done after all the required task has been executed
    """
    job = models.Job.query.get(job_id)
    if job.state != 'failed':
        job.state = 'done'
    models.db.session.commit()


def import_data(
    files,
    instance,
    backup_file,
    asynchronous=True,
    reload=True,
    custom_output_dir=None,
    skip_mimir=False,
    skip_2ed=False,
):
    """
    import the data contains in the list of 'files' in the 'instance'

    :param files: files to import
    :param instance: instance to receive the data
    :param backup_file: If True the files are moved to a backup directory, else they are not moved
    :param asynchronous: If True all jobs are run in background, else the jobs are run in sequence the function
     will only return when all of them are finish
    :param reload: If True kraken would be reload at the end of the treatment
    :param custom_output_dir: subdirectory for the nav file created. If not given, the instance default one is taken
    :param skip_mimir: skip importing data into mimir
    :param skip_2ed: skip inserting last_load_dataset files into ed database
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
    job.state = 'running'
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

    def process_ed2nav():
        models.db.session.add(job)
        models.db.session.commit()
        # We pass the job id to each tasks, but job need to be commited for having an id
        for action in actions:
            action.kwargs['job_id'] = job.id
        # Create binary file (New .nav.lz4)
        binarisation = [ed2nav.si(instance_config, job.id, custom_output_dir)]
        actions.append(chain(*binarisation))
        # Reload kraken with new data after binarisation (New .nav.lz4)
        if reload:
            actions.append(reload_data.si(instance_config, job.id))

        if not skip_mimir:
            for dataset in job.data_sets:
                actions.extend(send_to_mimir(instance, dataset.name, dataset.family_type))
        else:
            current_app.logger.info("skipping mimir import")

        actions.append(finish_job.si(job.id))

        # We should delete old backup directories related to this instance
        actions.append(purge_instance.si(instance.id, current_app.config['DATASET_MAX_BACKUPS_TO_KEEP']))
        if asynchronous:
            return chain(*actions).delay()
        else:
            # all job are run in sequence and import_data will only return when all the jobs are finish
            return chain(*actions).apply()

    if skip_2ed:
        # For skip_2ed, skip inserting last_load_dataset files into ed database
        return process_ed2nav()
    for _file in files:
        filename = None

        dataset = models.DataSet()
        # NOTE: for the moment we do not use the path to load the data here
        # but we'll need to refactor this to take it into account
        try:
            dataset.type, _ = utils.type_of_data(_file)
            dataset.family_type = utils.family_of_data(dataset.type)
        except Exception:
            if backup_file:
                move_to_backupdirectory(_file, instance_config.backup_directory)
            current_app.logger.debug(
                "Corrupted source file : {} moved to {}".format(_file, instance_config.backup_directory)
            )
            continue

        if dataset.type in task:
            if backup_file:
                filename = move_to_backupdirectory(_file, instance_config.backup_directory, manage_sp_char=True)
            else:
                filename = _file

            has_pt_planner_loki = (
                hasattr(instance, 'pt_planners_configurations') and "loki" in instance.pt_planners_configurations
            )
            if has_pt_planner_loki:
                loki_data_source = instance.pt_planners_configurations.get('loki', {}).get('data_source')
                if loki_data_source is not None:
                    if loki_data_source == "minio":
                        if dataset.type == "fusio":
                            actions.append(fusio2s3.si(instance_config, filename, dataset_uid=dataset.uid))
                        if dataset.type == "gtfs":
                            actions.append(gtfs2s3.si(instance_config, filename, dataset_uid=dataset.uid))
                    elif loki_data_source == "local" and dataset.type in ["fusio", "gtfs"]:
                        zip_file = zip_if_needed(filename)
                        dest = os.path.join(os.path.dirname(instance_config.target_file), "ntfs.zip")
                        shutil.copy(zip_file, dest)
                    else:
                        current_app.logger.debug(
                            "unknown loki data_source '{}' for coverage '{}'".format(
                                loki_data_source, instance.name
                            )
                        )

            actions.append(task[dataset.type].si(instance_config, filename, dataset_uid=dataset.uid))
        else:
            # unknown type, we skip it
            current_app.logger.debug("unknown file type: {} for file {}".format(dataset.type, _file))
            continue

        # currently the name of a dataset is the path to it
        dataset.name = filename
        dataset.state = "pending"
        models.db.session.add(dataset)
        job.data_sets.append(dataset)

    if actions:
        return process_ed2nav()


def send_to_mimir(instance, filename, family_type):
    """
    :param instance: instance to receive the data
    :param filename: file to inject towards mimir
    :param family_type: dataset's family type

    - create a job with a data_set
    - data injection towards mimir(stops2mimir, ntfs2mimir, poi2mimir)

    returns action list
    """

    # if mimir isn't setup do not try to import data for the autocompletion
    if not any([is_activate_autocomplete_version(2) or is_activate_autocomplete_version(7)]):
        return []

    # Bail out if the family type is not one that mimir deals with.
    if family_type not in ['pt', 'poi']:
        return []

    # This test is to avoid creating a new job if there is no action on mimir.
    if not (instance.import_ntfs_in_mimir or instance.import_stops_in_mimir):
        return []

    actions = []
    job = models.Job()
    job.instance = instance
    job.state = 'running'

    if is_activate_autocomplete_version(7):
        dataset_es7 = create_and_get_dataset(ds_type="fusio", family_type="mimir7", filename=filename)
        models.db.session.add(dataset_es7)
        job.data_sets.append(dataset_es7)

    if is_activate_autocomplete_version(2):
        dataset_es2 = create_and_get_dataset(ds_type="fusio", family_type="mimir", filename=filename)
        models.db.session.add(dataset_es2)
        job.data_sets.append(dataset_es2)

    models.db.session.add(job)
    models.db.session.commit()

    for version in (2, 7):
        if not is_activate_autocomplete_version(version):
            logging.getLogger(__name__).info("Disable import mimir version {}".format(version))
            continue
        ds = dataset_es7 if version == 7 else dataset_es2
        if family_type == 'pt':
            # Import ntfs in Mimir
            if instance.import_ntfs_in_mimir:
                actions.append(ntfs2mimir.si(instance.name, filename, version, job.id, dataset_uid=ds.uid))
            # Import stops in Mimir.
            # if we are loading pt data we might want to load the stops to autocomplete
            # This action is deprecated: https://github.com/hove-io/mimirsbrunn/blob/4430eed1d81247fffa7cf32ba675a9c5ad8b1cbe/documentation/components.md#stops2mimir
            if instance.import_stops_in_mimir and not instance.import_ntfs_in_mimir:
                actions.append(stops2mimir.si(instance.name, filename, version, job.id, dataset_uid=ds.uid))
        else:  # assume family_type == 'poi':
            actions.append(poi2mimir.si(instance.name, filename, version, job.id, dataset_uid=ds.uid))

    actions.append(finish_job.si(job.id))
    return actions


@celery.task()
def update_data():
    for instance in models.Instance.query_existing().all():
        current_app.logger.debug("Update data of : {}".format(instance.name))
        instance_config = None
        try:
            instance_config = load_instance_config(instance.name)
        except:
            current_app.logger.exception("impossible to load instance configuration for %s", instance.name)
            # Do not stop the task if only one instance is missing
            continue
        files = glob.glob(instance_config.source_directory + "/*")
        if files:
            import_data(files, instance, backup_file=True)


BANO_REGEXP = re.compile('.*bano.*')
COSMOGONY_REGEXP = re.compile('.*cosmogony.*')
OPEN_ADDRESSES_REGEXP = re.compile('.*csv')


def create_and_get_dataset(ds_type, family_type, filename):
    dataset = models.DataSet()
    dataset.family_type = family_type
    dataset.type = ds_type

    # currently the name of a dataset is the path to it
    dataset.name = filename
    return dataset


def type_of_autocomplete_data(filename):
    """
    return the type of autocomplete data of the files
    filename can be either a directory, a file or a list of files

    return can be:
        - 'bano'
        - 'osm'
        - 'cosmogony'
        - 'oa'
    """

    def files_type(files):
        # first we try fusio, because it can load fares too
        if any(f for f in files if BANO_REGEXP.match(f)):
            return 'bano'
        if len(files) == 1 and COSMOGONY_REGEXP.match(files[0]):
            return 'cosmogony'
        if len(files) == 1 and files[0].endswith('.pbf'):
            return 'osm'
        # OpenAddresses files does not have a predefined naming,
        # so we check it last, and consider all csv as OA
        if any(f for f in files if OPEN_ADDRESSES_REGEXP.match(f)):
            return 'oa'
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
def import_autocomplete(files, autocomplete_instance, asynchronous=True, backup_file=True):
    """
    Import the autocomplete'instance data files
    """
    job = models.Job()
    job.state = 'running'
    actions = []
    task = {
        'bano': {2: bano2mimir, 7: bano2mimir},
        'oa': {2: openaddresses2mimir, 7: openaddresses2mimir},
        'osm': {2: osm2mimir, 7: osm2mimir},
        'cosmogony': {2: cosmogony2mimir, 7: cosmogony2mimir},
    }
    autocomplete_dir = current_app.config['AUTOCOMPLETE_DIR']

    # it's important for the admin to be loaded first, then addresses, then street, then poi
    import_order = ['cosmogony', 'bano', 'oa', 'osm']
    files_and_types = [(f, type_of_autocomplete_data(f)) for f in files]
    files_and_types = sorted(files_and_types, key=lambda f_t: import_order.index(f_t[1]))

    for f, ftype in files_and_types:
        if ftype not in task:
            # unknown type, we skip it
            current_app.logger.debug("unknown file type: {} for file {}".format(ftype, f))
            continue
        filename = f
        if backup_file:
            filename = move_to_backupdirectory(
                f, autocomplete_instance.backup_dir(autocomplete_dir), manage_sp_char=True
            )

        for version, executable in task[ftype].items():
            if not is_activate_autocomplete_version(version):
                current_app.logger.debug("Autocomplete version {} is disableed".format(version))
                continue
            dataset = create_and_get_dataset(
                ds_type=ftype, family_type='autocomplete_{}'.format(ftype), filename=filename
            )
            actions.append(
                executable.si(
                    autocomplete_instance,
                    filename=filename,
                    job_id=job.id,
                    dataset_uid=dataset.uid,
                    autocomplete_version=version,
                )
            )
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
    if asynchronous:
        return chain(*actions).delay(), job
    else:
        # all job are run in sequence and import_data will only return when all the jobs are finish
        return chain(*actions).apply(), job


@celery.task()
def import_in_mimir(_file, instance, asynchronous=True):
    """
    Import pt data stops to autocomplete
    """
    datatype, _ = utils.type_of_data(_file)
    family_type = utils.family_of_data(datatype)
    current_app.logger.debug("Import {} data to mimir".format(family_type))

    actions = []

    for version in (2, 7):
        if not is_activate_autocomplete_version(version):
            logging.getLogger(__name__).info("Disable import mimir version {}".format(version))
            continue
        if family_type == 'pt':
            if instance.import_ntfs_in_mimir:
                actions.append(ntfs2mimir.si(instance.name, _file, version))
            # Deprecated: https://github.com/hove-io/mimirsbrunn/blob/4430eed1d81247fffa7cf32ba675a9c5ad8b1cbe/documentation/components.md#stops2mimir
            if instance.import_stops_in_mimir and not instance.import_ntfs_in_mimir:
                actions.append(stops2mimir.si(instance.name, _file, version))
        elif family_type == 'poi':
            actions.append(poi2mimir.si(instance.name, _file, version))
        else:
            current_app.logger.warning("Unsupported family_type {}".format(family_type))

    if asynchronous:
        return chain(*actions).delay()
    else:
        # all job are run in sequence and import_in_mimir will only return when all the jobs are finish
        return chain(*actions).apply()


@celery.task()
def update_autocomplete():
    current_app.logger.debug("Update autocomplete data")
    autocomplete_dir = current_app.config['AUTOCOMPLETE_DIR']
    for autocomplete_instance in models.AutocompleteParameter.query.all():
        files = glob.glob(autocomplete_instance.source_dir(autocomplete_dir) + "/*")
        if files:
            import_autocomplete(files, autocomplete_instance, backup_file=True)


@celery.task()
def purge_datasets():
    instances = models.Instance.query_existing().all()
    current_app.logger.info("Instances to purge: {}".format(instances))
    for instance in instances:
        try:
            purge_instance(instance.id, current_app.config['DATASET_MAX_BACKUPS_TO_KEEP'])
        except Exception as e:
            # Do not stop the task for all other instances if only one instance is missing
            current_app.logger.error("Dataset purge failed for instance {i}: {e}".format(i=instance, e=e))


@celery.task()
def purge_instance(instance_id, nb_to_keep):
    instance = models.Instance.query.get(instance_id)
    logger = get_instance_logger(instance)
    logger.info('purge of backup directories for %s', instance.name)
    try:
        instance_config = load_instance_config(instance.name)
    except Exception as e:
        logger.error("Impossible to load instance configuration for {i}: {e}".format(i=instance.name, e=e))
        return

    backups = set(glob.glob('{}/*'.format(instance_config.backup_directory)))
    logger.info('backups are: %s', backups)
    # we add the realpath not to have problems with double / or stuff like that
    loaded = set(
        os.path.realpath(os.path.dirname(dataset.name)) for dataset in instance.last_datasets(nb_to_keep)
    )
    logger.info('loaded  data are: %s', loaded)

    running = set(os.path.realpath(os.path.dirname(dataset.name)) for dataset in instance.running_datasets())
    logger.info('running  bina are: %s', running)
    to_remove = [os.path.join(instance_config.backup_directory, f) for f in backups - loaded - running]

    missing = [l for l in loaded if l not in backups]
    if missing:
        logger.error(
            "MISSING backup files! impossible to find %s in the backup dir, "
            "we skip the purge, repair ASAP to fix the purge",
            missing,
        )
        return

    logger.info('we remove: %s', to_remove)
    for path in to_remove:
        shutil.rmtree(path)


def purge_cities():
    """
    Delete old 'cities' jobs and the associated dataset in db and on disk
    """
    nb_datasets_to_keep = current_app.config.get('DATASET_MAX_BACKUPS_TO_KEEP', 1)
    cities_job = (
        models.Job.query.join(models.DataSet)
        .filter(models.DataSet.type == 'cities')
        .order_by(models.Job.created_at.desc())
        .all()
    )
    cities_job_to_keep = cities_job[:nb_datasets_to_keep]
    datasets_to_keep = [job.data_sets.first().name for job in cities_job_to_keep]

    for job in cities_job[nb_datasets_to_keep:]:
        logging.info(" - Remove JOB {}".format(job.id))
        dataset = job.data_sets.first()
        logging.info("   Remove associated DATASET {}".format(dataset.id))
        models.db.session.delete(dataset)
        if os.path.exists(dataset.name) and dataset.name not in datasets_to_keep:
            logging.info("    - delete file {}".format(dataset.name))
            shutil.rmtree('{}'.format(dataset.name))
        models.db.session.delete(job)

    models.db.session.commit()


@celery.task()
def purge_jobs(days_to_keep=None):
    """
    Delete old jobs in database and backup folders associated
    :param days_to_keep: Period of time to keep jobs (in days). The default value is 'JOB_MAX_PERIOD_TO_KEEP'
    """
    if days_to_keep is None:
        days_to_keep = current_app.config.get('JOB_MAX_PERIOD_TO_KEEP', 60)

    time_limit = datetime.utcnow() - timedelta(days=int(days_to_keep))
    # Purge all instances (even discarded = true)
    instances = models.Instance.query_all().all()

    logger = logging.getLogger(__name__)
    logger.info('Purge old jobs and datasets backup created before {}'.format(time_limit))

    for instance in instances:
        datasets_to_delete = instance.delete_old_jobs_and_list_datasets(time_limit)

        if datasets_to_delete:
            backups_to_delete = set(os.path.realpath(os.path.dirname(dataset)) for dataset in datasets_to_delete)
            logger.info('backups_to_delete are: {}'.format(backups_to_delete))

            for path in backups_to_delete:
                if os.path.exists(path):
                    shutil.rmtree('{}'.format(path))
                else:
                    logger.warning('Folder {} can\'t be found'.format(path))

    # Purge 'cities' jobs (which aren't associated to an instance)
    purge_cities()


@celery.task()
def scan_instances():
    for instance_name in get_instances_name():
        instance = models.Instance.query_all().filter_by(name=instance_name).first()
        if not instance:
            current_app.logger.info('new instances detected: %s', instance_name)
            instance = models.Instance(name=instance_name)
            instance_config = load_instance_config(instance.name)
            instance.is_free = instance_config.is_free
            # by default we will consider an free instance as an opendata one
            instance.is_open_data = instance_config.is_free

            models.db.session.add(instance)
            models.db.session.commit()


@celery.task()
def reload_kraken(instance_id):
    instance = models.Instance.query.get(instance_id)
    job = models.Job()
    job.instance = instance
    job.state = 'running'
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
    job.state = 'running'
    instance_config = load_instance_config(instance.name)
    models.db.session.add(job)
    models.db.session.commit()
    chain(ed2nav.si(instance_config, job.id, None), finish_job.si(job.id)).delay()
    current_app.logger.info("Job build data of : %s queued" % instance.name)


@celery.task()
def load_data(instance_id, data_dirs):
    instance = models.Instance.query.get(instance_id)

    import_data(data_dirs, instance, backup_file=False, asynchronous=False)


@celery.task()
def cities(file_path, job_id, exe):
    """Launch 'cities' or 'cosmogony2cities'"""
    job = models.Job.query.get(job_id)
    res = -1
    try:
        res = launch_exec(
            "{}".format(exe),
            ['-i', file_path, '--connection-string', current_app.config['CITIES_DATABASE_URI']],
            logging,
        )
        if res != 0:
            job.state = 'failed'
            logging.error('{} failed'.format(exe))
        else:
            job.state = 'done'

    except Exception as e:
        logging.exception('{} exception : {}'.format(exe, e.message))
        job.state = 'failed'
        models.db.session.commit()
        raise

    models.db.session.commit()
    logging.info('Import of {} finished'.format(exe))
    return res


@celery.task()
def bounding_shape(instance_name, shape_path):
    """Set the bounding shape to a custom value"""

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
    with kombu.Connection(current_app.config['KRAKEN_BROKER_URL']) as connection:
        instances = models.Instance.query_existing().all()
        task = task_pb2.Task()
        task.action = task_pb2.HEARTBEAT

        for instance in instances:
            try:
                config = load_instance_config(instance.name)
                exchange = kombu.Exchange(config.exchange, 'topic', durable=True)
                producer = connection.Producer(exchange=exchange)
                producer.publish(task.SerializeToString(), routing_key='{}.task.heartbeat'.format(instance.name))
            except Exception as e:
                logging.error("Could not ping krakens for instance {i}: {e}".format(i=instance, e=e))


@celery.task()
def create_autocomplete_depot(name):
    autocomplete_dir = current_app.config['AUTOCOMPLETE_DIR']
    autocomplete = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
    create_autocomplete_instance_paths(autocomplete)


@celery.task()
def remove_autocomplete_depot(name):
    logging.info('removing instance dir for {}'.format(name))
    autocomplete_dir = current_app.config['AUTOCOMPLETE_DIR']
    if os.path.exists(autocomplete_dir):
        autocomplete = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        main_dir = autocomplete.main_dir(autocomplete_dir)
        if os.path.exists(main_dir):
            shutil.rmtree(main_dir)
        else:
            logging.warning('no autocomplete directory for {}, removing nothing'.format(autocomplete_dir))
    else:
        logging.warning('no main autocomplete directory, removing nothing')


@celery.task()
def purge_autocomplete():
    logger = logging.getLogger(__name__)
    autocomplete_instances = models.AutocompleteParameter.query.all()
    for ac_instance in autocomplete_instances:
        logger.info('purging autocomplete backup directories for %s', ac_instance.name)
        max_backups = current_app.config.get('AUOTOCOMPLETE_MAX_BACKUPS_TO_KEEP', 5)
        dir_to_keep = set(
            os.path.realpath(os.path.dirname(dataset.name)) for dataset in ac_instance.last_datasets(max_backups)
        )
        autocomplete_dir = current_app.config['AUTOCOMPLETE_DIR']
        backup_dir = os.path.join(autocomplete_dir, ac_instance.name, 'backup')
        all_backups = set(os.path.join(backup_dir, backup) for backup in os.listdir(backup_dir))
        to_remove = all_backups - dir_to_keep
        for directory in to_remove:
            if os.path.exists(directory):
                try:
                    logger.info('removing backup directory: %s', directory)
                    shutil.rmtree(directory)
                except Exception as e:
                    logger.info('cannot purge directory: %s because: %s', directory, str(e))
