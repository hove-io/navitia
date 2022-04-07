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

from flask_script import Command, Option
from navitiacommon import models
from tyr.tasks import load_data
import logging


class LoadDataCommand(Command):
    """A command used for run trigger a reload of data from a directory"""

    def get_options(self):
        return [
            Option(dest='instance_name', help="name of the instance"),
            Option(dest='data_dir', help="coma separated list of the path of the data to load"),
        ]

    def run(self, instance_name, data_dir):
        logging.info("Run command load data, a tyr_worker has to be " "previously started")
        instance = models.Instance.query_existing().filter_by(name=instance_name).first()

        if not instance:
            raise Exception("cannot find instance {}".format(instance_name))

        data_dirs = data_dir.split(',')
        logging.info("loading data dir : {}".format(data_dirs))

        # this api is only used in artemis and artemis wants to know when this
        # task is finished, so the easiest way for the moment is to wait for
        # the answer
        load_data(instance.id, data_dirs)
