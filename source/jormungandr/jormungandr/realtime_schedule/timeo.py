# coding=utf-8

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
import requests as requests
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy


class Timeo(RealtimeProxy):
    """
    class managing calls to timeo external service providing real-time next passages
    """

    def __init__(self, service_url):
        self.service_url = service_url

    def next_passage_for_route_point(self, route_point):
        url = self.get_url(route_point)
        r = requests.get(url)
        if r.status_code != 200:
            raise Exception("Timeo RT service unavailable")

        return self.get_passages(r.json())

    def get_url(self, route_point):
        #TODO implem
        return ""

    def get_passages(timeo_resp):
        #TODO implem
        return []
