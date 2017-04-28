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

from __future__ import absolute_import, print_function, unicode_literals, division
from flask.ext.restful import Resource, fields, marshal_with
from jormungandr import i_manager, travelers_profile
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr.interfaces.v1.fields import instance_status_with_parameters

status = {
    "status": fields.Nested(instance_status_with_parameters)
}


class Status(Resource):
    @marshal_with(status)
    def get(self, region):
        response = protobuf_to_dict(i_manager.dispatch({}, "status", instance_name=region), use_enum_labels=True)
        instance = i_manager.instances[region]
        response['status']["is_open_data"] = instance.is_open_data
        response['status']["is_open_service"] = instance.is_free
        response['status']['parameters'] = instance
        response['status']['traveler_profiles'] = travelers_profile.TravelerProfile.get_profiles_by_coverage(region)
        response['status']['realtime_proxies'] = []
        for realtime_proxy in instance.realtime_proxy_manager.realtime_proxies.values():
            response['status']['realtime_proxies'].append(realtime_proxy.status())
        return response, 200
