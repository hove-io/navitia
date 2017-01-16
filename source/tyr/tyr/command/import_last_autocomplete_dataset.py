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

from navitiacommon import models
from tyr import tasks
import logging
from tyr import manager


@manager.command
def import_last_autocomplete_dataset(instance_name, wait=True):
    """
    reimport the last datasets of an autocomplete instance

    If not given, the instance default one is taken
    By default job is not run on the workers, you need to pass --background for use them, in that case you can
    also pass --nowait for not waiting the end of the job

    """
    instance = models.AutocompleteParameter.query.filter_by(name=instance_name).first()

    if not instance:
        raise Exception("cannot find autocomplete instance {}".format(instance_name))

    files = [d.name for d in instance.last_datasets(1)]
    logger = logging.getLogger(__name__)
    logger.info('we reimport the last dataset of autocomplete %s, composed of: %s', instance_name, files)
    future = tasks.import_autocomplete(files, instance, backup_file=False)
    if wait and future:
        future.wait()

    logger.info('last datasets reimport finished for %s', instance_name)


@manager.command
def import_all_last_autocomplete_dataset(wait=True):
    instances_name = [i.name for i in models.AutocompleteParameter.query.all()]
    for name in instances_name:
        import_last_autocomplete_dataset(instance_name=name, wait=wait)

    # TODO we need to find a way to reimport also the public transport dataset in the autocomplete
