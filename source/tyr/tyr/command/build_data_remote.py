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

from flask_script import Command, Option
from tyr.tasks import build_all_data, build_data_with_instance_name
import logging


class BuildDataRemoteCommand(Command):
    """A command used to build all the datasets REMOTELY"""

    def get_options(self):
        return [
            Option(
                '-n',
                '--name',
                dest='instance_name',
                help="name of the instance to build. If non given, build all instances",
                default=None,
            ),
            Option('-a', '--all', dest='all_instances', action="store_true", help="build all instances"),
        ]

    def run(self, instance_name=None, all_instances=False):
        if all_instances:
            logging.info("Launching ed2nav for all instances")
            return build_all_data.delay()

        logging.info("Launching ed2nav for instance: {}".format(instance_name))
        return build_data_with_instance_name.delay(instance_name)
