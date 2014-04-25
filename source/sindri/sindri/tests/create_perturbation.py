#!/usr/bin/env python

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

import kombu
from navitiacommon import task_pb2
import datetime


connection = kombu.Connection()
exchange = kombu.Exchange('navitia', 'topic', durable=True)
producer = connection.Producer(exchange=exchange)

task = task_pb2.Task()
task.action = 2
task.at_perturbation.uri = 'testajn2'
#task.at_perturbation.object.object_uri = 'line:SIT10'
#task.at_perturbation.object.object_type = 1
task.at_perturbation.object.object_uri = 'stop_area:SIT150'
task.at_perturbation.object.object_type = 5
task.at_perturbation.start_application_date = 0
task.at_perturbation.end_application_date = 1390033727

task.at_perturbation.start_application_daily_hour = 0
task.at_perturbation.end_application_daily_hour = 86399

# for i in range(10000):
producer.publish(task.SerializeToString(), routing_key='realtime.at')

connection.close()
