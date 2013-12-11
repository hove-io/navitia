from celery import chain
from celery.signals import task_postrun
from tyr.binarisation import gtfs2ed, osm2ed, ed2nav, nav2rt
from tyr.binarisation import reload_data, move_to_backupdirectory
from flask import current_app
import glob
from tyr.app import celery, db
from tyr import models
import os


@celery.task()
def update_data():
    for instance in models.Instance.query.all():
        instance.load_technical_config()
        osm_files = glob.glob(instance.source_directory + "/*.pbf")
        gtfs_files = glob.glob(instance.source_directory + "/*.zip")
        actions = []
        job = models.Job()
        job.instance = instance
        #we only handle one datatype in a task
        #so if we depose two files at the same time, two task would be created
        if gtfs_files:
            filename = move_to_backupdirectory(gtfs_files[0], instance)
            actions.append(gtfs2ed.si(instance, filename))
            job.type = 'gtfs'
            job.filename = filename
        elif osm_files:
            filename = move_to_backupdirectory(osm_files[0], instance)
            actions.append(osm2ed.si(instance, filename))
            job.type = 'osm'
            job.filename = filename

        if actions:
            actions.append(ed2nav.si(instance))
            actions.append(nav2rt.si(instance))
            actions.append(reload_data.si(instance))
            #We start the task in one seconds so we can save the job in db
            task = chain(*actions).delay(countdown=1)
            job.task_uuid = task.id
            db.session.add(job)
            db.session.commit()


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
            db.session.add(instance)
            db.session.commit()

@task_postrun.connect
def close_session(*args, **kwargs):
    # Flask SQLAlchemy will automatically create new sessions for you from
    # a scoped session factory, given that we are maintaining the same app
    # context, this ensures tasks have a fresh session (e.g. session errors
    # won't propagate across tasks)
    db.session.remove()
