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

from flask.ext.script import Command, Option
from navitiacommon import models
from tyr.helper import load_instance_config
from tyr.aggregate_places import aggregate_places
import logging

class AggregatePlacesCommand(Command):
    """A command used in development environment to run aggregate places without
    having to run tyr
    """

    def get_options(self):
        return [
            Option('-n', '--name', dest='name_', default="",
                   help="Launch aggregate places, if no name is specified "
                   "this is launched for all instances")
        ]

    def run(self, name_=""):
        if name_:
            instances = models.Instance.query.filter_by(name=name_).all()
        else:
            instances = models.Instance.query.all()

        if not instances:
            logging.getLogger(__name__).\
                error("Unable to find any instance for name '{name}'"
                      .format(name=name_))
            return

        #TODO: create a real job
        job_id = 1
        instances_name = [instance.name for instance in instances]
        for instance_name in instances_name:
            instance_config = None
            try:
                instance_config = load_instance_config(instance_name)
            except ValueError:
                logging.getLogger(__name__).\
                    info("Unable to find instance " + instance_name)
                continue
            aggregate_places(instance_config, job_id)
            job_id += 1


