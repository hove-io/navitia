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
import logging
from tyr import manager
from tyr.helper import get_instance_logger, wait_or_raise


@manager.command
def import_last_dataset(
    instance_name,
    background=False,
    reload_kraken=False,
    custom_output_dir=None,
    nowait=False,
    allow_mimir=False,
    skip_2ed=False,
):
    """
    reimport the last dataset of a instance
    By default the kraken is not reloaded, the '-r' switch can activate it

    the custom_output_dir parameter is a subdirectory for the nav file created.
    If not given, the instance default one is taken
    By default job is not run on the workers, you need to pass --background for use them, in that case you can
    also pass --nowait for not waiting the end of the job

    """
    instance = models.Instance.query_existing().filter_by(name=instance_name).first()
    # little trick to keep a flag on the command line: -a enable mimir import,
    # we reverse it to match import_data
    skip_mimir = not allow_mimir

    if not instance:
        raise Exception("cannot find instance {}".format(instance_name))

    files = [d.name for d in instance.last_datasets(1)]
    logger = get_instance_logger(instance)
    logger.info('we reimport the last dataset of %s, composed of: %s', instance.name, files)
    future = tasks.import_data(
        files,
        instance,
        backup_file=False,
        asynchronous=background,
        reload=reload_kraken,
        custom_output_dir=custom_output_dir,
        skip_mimir=skip_mimir,
        skip_2ed=skip_2ed,
    )
    if not nowait and future:
        wait_or_raise(future)
