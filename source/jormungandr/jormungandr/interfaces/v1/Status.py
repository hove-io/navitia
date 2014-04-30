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

status = {
    "status": fields.Nested( {
        "data_version": fields.Integer(),
        "end_production_date": fields.String(),
        "is_connected_to_rabbitmq": fields.Boolean(),
        "last_load_at": fields.String(),
        "last_load_status": fields.Boolean(),
        "kraken_version": fields.String(attribute="navitia_version"),
        "nb_threads": fields.Integer(),
        "publication_date": fields.String(),
        "start_production_date": fields.String()
    })
}
class Status(Resource):
    @marshal_with(status)
    def get(self, region):
        response = i_manager.dispatch([], "status", instance_name=region)
        return protobuf_to_dict(response, use_enum_labels=True), 200
