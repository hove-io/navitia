# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from tyr import manager


@manager.command
def purge_instance(instance_name, nb_to_keep, background=False):
    """
    remove old backup directories for one instance for only keeping the data being actually loaded
    """
    instance = models.Instance.query_existing().filter_by(name=instance_name).first()

    if not instance:
        raise Exception("cannot find instance {}".format(instance_name))
    if background:
        tasks.purge_instance.delay(instance.id, nb_to_keep)
    else:
        tasks.purge_instance(instance.id, nb_to_keep)


@manager.command
def purge_instances(nb_to_keep, background=False):
    """
    remove old backup directories of all instances for only keeping the data being actually loaded
    """
    for instance in models.Instance.query_existing().all():
        if background:
            tasks.purge_instance.delay(instance.id, nb_to_keep)
        else:
            tasks.purge_instance(instance.id, nb_to_keep)


@manager.command
def purge_old_jobs(days_to_keep=None, background=False):
    """
    Delete old jobs in database and backup folders associated
    """
    if background:
        tasks.purge_jobs.delay(days_to_keep)
    else:
        tasks.purge_jobs(days_to_keep)
