from celery import chain, group
from celery.signals import task_postrun
from tyr.binarisation import gtfs2ed, osm2ed, ed2nav, nav2rt, fusio2ed, geopal2ed, fare2ed
from tyr.binarisation import reload_data, move_to_backupdirectory
from tyr.aggregate_places import aggregate_places
from flask import current_app
import glob
from tyr import celery
from navitiacommon import models
import os
import zipfile
from tyr.helper import load_instance_config


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
        if 'fares.csv' in zipf.namelist():
            return 'fare'

        if "contributors.txt" in zipf.namelist():
            return 'fusio'
        else:
            return 'gtfs'
    if filename.endswith('.geopal'):
        return 'geopal'
    return None


@celery.task()
def finish_job(job_id):
    """
    use for mark a job as done after all the required task has been executed
    """
    job = models.Job.query.get(job_id)
    job.state = 'done'
    models.db.session.commit()


@celery.task()
def update_data():
    for instance in models.Instance.query.all():
        current_app.logger.info("Update data of : %s"%instance.name)
        instance_config = load_instance_config(instance.name)
        files = glob.glob(instance_config.source_directory + "/*")
        actions = []
        job = models.Job()
        job.instance = instance
        job.state = 'pending'
        for _file in files:
            dataset = models.DataSet()
            filename = None

            dataset.type = type_of_data(_file)
            if dataset.type == 'gtfs':
                filename = move_to_backupdirectory(_file,
                        instance_config.backup_directory)
                actions.append(gtfs2ed.si(instance_config, filename))
            elif dataset.type == 'fusio':
                filename = move_to_backupdirectory(_file,
                        instance_config.backup_directory)
                actions.append(fusio2ed.si(instance_config,
                                           filename))
            elif dataset.type == 'osm':
                filename = move_to_backupdirectory(_file,
                        instance_config.backup_directory)
                actions.append(osm2ed.si(instance_config, filename))
            elif dataset.type == 'geopal':
                filename = move_to_backupdirectory(_file,
                        instance_config.backup_directory)
                actions.append(geopal2ed.si(instance_config, filename))
            elif dataset.type == 'fare':
                filename = move_to_backupdirectory(_file,
                        instance_config.backup_directory)
                actions.append(fare2ed.si(instance_config, filename))
            else:
                #unknown type, we skip it
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
            binarisation = [ed2nav.si(instance_config, job.id),
                            nav2rt.si(instance_config, job.id)]
            aggregate = aggregate_places.si(instance_config, job.id)
            #We pass the job id to each tasks, but job need to be commited for
            #having an id
            actions.append(group(chain(*binarisation), aggregate))
            actions.append(reload_data.si(instance_config, job.id))
            actions.append(finish_job.si(job.id))
            chain(*actions).delay()



@celery.task()
def scan_instances():
    for instance_file in glob.glob(current_app.config['INSTANCES_DIR'] \
            + '/*.ini'):
        instance_name = os.path.basename(instance_file).replace('.ini', '')
        instance = models.Instance.query.filter_by(name=instance_name).first()
        if not instance:
            current_app.logger.info('new instances detected: %s',
                    instance_name)
            instance = models.Instance(name=instance_name)
            models.db.session.add(instance)
            models.db.session.commit()


@task_postrun.connect
def close_session(*args, **kwargs):
    # Flask SQLAlchemy will automatically create new sessions for you from
    # a scoped session factory, given that we are maintaining the same app
    # context, this ensures tasks have a fresh session (e.g. session errors
    # won't propagate across tasks)
    models.db.session.remove()
