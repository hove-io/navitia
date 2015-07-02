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

from flask.ext.restful import Resource, fields, marshal_with
from jormungandr import i_manager
from jormungandr.protobuf_to_dict import protobuf_to_dict
from fields import instance_status_with_parameters
from jormungandr import app
from navitiacommon import models

status = {
    "status": fields.Nested(instance_status_with_parameters)
}


class Status(Resource):
    @marshal_with(status)
    def get(self, region):
        response = protobuf_to_dict(i_manager.dispatch({}, "status", instance_name=region), use_enum_labels=True)
        instance = i_manager.instances[region]
        is_open_data = True
        if not app.config['DISABLE_DATABASE']:
            if instance and instance.name:
                instance_db = models.Instance.get_by_name(instance.name)
                if instance_db:
                    is_open_data = instance_db.is_free
        response['status']["is_open_data"] = is_open_data
        response['status']['parameters'] = instance
        return response, 200
