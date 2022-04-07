# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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

from navitiacommon import models
from tyr import tasks
import logging
from tyr import manager

from tyr.helper import wait_or_raise


@manager.command
def import_last_autocomplete_dataset(instance_name, wait=True):
    """
    reimport the last datasets of an autocomplete instance

    you can set wait=False if you don't want to wait for the result
    """
    instance = models.AutocompleteParameter.query.filter_by(name=instance_name).first()

    if not instance:
        raise Exception("cannot find autocomplete instance {}".format(instance_name))

    files = [d.name for d in instance.last_datasets(1)]
    logger = logging.getLogger(__name__)
    logger.info('we reimport the last dataset of autocomplete %s, composed of: %s', instance_name, files)
    future, _ = tasks.import_autocomplete(files, instance, backup_file=False)
    if wait and future:
        wait_or_raise(future)

    logger.info('last datasets reimport finished for %s', instance_name)


@manager.command
def import_last_stop_dataset(instance_name, wait=True):
    """
    reimport the last datasets of of all instances with import_stops_in_mimir = true

    you can set wait=False if you don't want to wait for the result
    """
    instance = models.Instance.query_existing().filter_by(name=instance_name).first()

    if not instance:
        raise Exception("cannot find instance {}".format(instance_name))

    files = [d.name for d in instance.last_datasets(nb_dataset=1, family_type='pt')]
    files.extend([d.name for d in instance.last_datasets(nb_dataset=1, family_type='poi')])
    logger = logging.getLogger(__name__)
    logger.info('we reimport to mimir the last dataset of %s, composed of: %s', instance.name, files)
    if len(files) >= 1:
        for _file in files:
            future = tasks.import_in_mimir(_file, instance)
            if wait and future:
                wait_or_raise(future)
            logger.info('last datasets reimport finished for %s', instance.name)
    else:
        logger.info('No file to reimport to mimir the last dataset of %s', instance.name)


@manager.command
def refresh_autocomplete_data(wait=True):
    """
    reimport all the last autocomplete datas along with the last dataset of all instances
    with import_stops_in_mimir = true

    """
    instances_name = [i.name for i in models.AutocompleteParameter.query.all()]
    for name in instances_name:
        import_last_autocomplete_dataset(instance_name=name, wait=wait)

    # we need to reimport also the public transport dataset in the autocomplete
    navitia_instance = models.Instance.query_existing().filter_by(import_stops_in_mimir=True).all()
    for name in [i.name for i in navitia_instance]:
        import_last_stop_dataset(instance_name=name, wait=wait)
