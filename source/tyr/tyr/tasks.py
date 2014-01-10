from celery import chain
from celery.signals import task_postrun
from tyr.binarisation import gtfs2ed, osm2ed, ed2nav, nav2rt, fusio2ed
from tyr.binarisation import reload_data, move_to_backupdirectory
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
        if "contributors.txt" in zipf.namelist():
            return 'fusio'
        else:
            return 'gtfs'
    return None

@celery.task()
def update_data():
    for instance in models.Instance.query.all():
        instance_config = load_instance_config(instance.name)
        files = glob.glob(instance_config.source_directory + "/*")
        actions = []
        job = models.Job()
        #we only handle one datatype in a task
        #so if we depose two files at the same time, two task would be created
        for _file in files:
            #TODO: job must be split in two, first the job/task for monitoring
            # its progress and a second entities with the datas processed by
            # this task
            filename = move_to_backupdirectory(_file,
                    instance_config.backup_directory)
            job.filename = filename
            job.instance = instance
            job.type = type_of_data(job.filename)
            if job.type == 'gtfs':
                actions.append(gtfs2ed.si(instance, instance_config, filename))
            elif job.type == 'fusio':
                actions.append(fusio2ed.si(instance, instance_config,
                                           filename))
            elif job.type == 'osm':
                actions.append(osm2ed.si(instance, instance_config, filename))

        if actions:
            actions.append(ed2nav.si(instance, instance_config))
            actions.append(nav2rt.si(instance, instance_config))
            actions.append(reload_data.si(instance, instance_config))
            #We start the task in one seconds so we can save the job in db
            # it will be better to pass the job to the task
            task = chain(*actions).delay(countdown=1)
            job.task_uuid = task.id
            models.db.session.add(job)
            models.db.session.commit()


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
