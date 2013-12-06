from celery import chain
from pyed.binarisation import gtfs2ed, osm2ed, ed2nav, nav2rt, reload_data, move_to_backupdirectory
from flask import current_app
import glob
from pyed.app import celery, redis
import os



@celery.task()
def scan():
    for instance in current_app.instances.values():
        osm_files = glob.glob(instance.source_directory+"/*.pbf")
        gtfs_files = glob.glob(instance.source_directory+"/*.zip")
        actions = []
        if gtfs_files:
            filename = move_to_backupdirectory(gtfs_files[0], instance)
            actions.append(gtfs2ed.si(instance, filename))

        if osm_files:
            filename = move_to_backupdirectory(osm_files[0], instance)
            actions.append(osm2ed.si(instance, filename))

        if actions:
            actions.append(ed2nav.si(instance))
            actions.append(nav2rt.si(instance))
            actions.append(reload_data.si(instance))
            chain(*actions).delay()




