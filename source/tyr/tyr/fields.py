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

from flask_restful import fields
from flask.globals import g
import ujson


class FieldDate(fields.Raw):
    def format(self, value):
        if value:
            return value.isoformat()
        else:
            return 'null'


class HasShape(fields.Raw):
    def output(self, key, obj):
        return obj.has_shape()


class Shape(fields.Raw):
    def __init__(self, **kwargs):
        super(Shape, self).__init__(**kwargs)

    def output(self, key, obj):
        if obj.shape == 'null':
            obj.shape = None

        if hasattr(g, 'disable_geojson') and g.disable_geojson and obj.has_shape():
            return {}

        if obj.shape is not None:
            return ujson.loads(obj.shape)

        return obj.shape


end_point_fields = {
    'id': fields.Raw,
    'name': fields.Raw,
    'default': fields.Raw,
    'hostnames': fields.List(fields.String)
}

key_fields = {
    'id': fields.Raw,
    'app_name': fields.Raw,
    'token': fields.Raw,
    'valid_until': FieldDate
}

instance_fields = {
    'id': fields.Raw,
    'name': fields.Raw,
    'discarded': fields.Raw,
    'is_free': fields.Raw,
    'import_stops_in_mimir': fields.Raw,
    'scenario': fields.Raw,
    'journey_order': fields.Raw,
    'max_walking_duration_to_pt': fields.Raw,
    'max_bike_duration_to_pt': fields.Raw,
    'max_bss_duration_to_pt': fields.Raw,
    'max_car_duration_to_pt': fields.Raw,
    'max_nb_transfers': fields.Raw,
    'walking_speed': fields.Raw,
    'bike_speed': fields.Raw,
    'bss_speed': fields.Raw,
    'car_speed': fields.Raw,
    'min_bike': fields.Raw,
    'min_bss': fields.Raw,
    'min_car': fields.Raw,
    'min_tc_with_bike': fields.Raw,
    'min_tc_with_bss': fields.Raw,
    'min_tc_with_car': fields.Raw,
    'max_duration_criteria': fields.Raw,
    'max_duration_fallback_mode': fields.Raw,
    'max_duration': fields.Raw,
    'walking_transfer_penalty': fields.Raw,
    'night_bus_filter_max_factor': fields.Raw,
    'night_bus_filter_base_factor': fields.Raw,
    'priority': fields.Raw,
    'bss_provider': fields.Boolean,
    'full_sn_geometries': fields.Boolean,
    'is_open_data': fields.Boolean,
}

api_fields = {
    'id': fields.Raw,
    'name': fields.Raw
}

billing_plan_fields = {
    'id': fields.Raw,
    'name': fields.Raw,
    'max_request_count': fields.Raw,
    'max_object_count': fields.Raw,
    'default': fields.Raw,
}

billing_plan_fields_full = {
    'id': fields.Raw,
    'name': fields.Raw,
    'max_request_count': fields.Raw,
    'max_object_count': fields.Raw,
    'default': fields.Raw,
    'end_point': fields.Nested(end_point_fields)
}

user_fields = {
    'id': fields.Raw,
    'login': fields.Raw,
    'email': fields.Raw,
    'block_until': FieldDate,
    'type': fields.Raw(),
    'end_point': fields.Nested(end_point_fields),
    'billing_plan': fields.Nested(billing_plan_fields),
    'has_shape': HasShape,
    'shape': Shape
}

user_fields_full = {
    'id': fields.Raw,
    'login': fields.Raw,
    'email': fields.Raw,
    'block_until': FieldDate,
    'type': fields.Raw(),
    'keys': fields.List(fields.Nested(key_fields)),
    'authorizations': fields.List(fields.Nested({
        'instance': fields.Nested(instance_fields),
        'api': fields.Nested(api_fields)
    })),
    'end_point': fields.Nested(end_point_fields),
    'billing_plan': fields.Nested(billing_plan_fields),
    'has_shape': HasShape,
    'shape': Shape
}

dataset_field = {
    'type': fields.Raw,
    'name': fields.Raw,
    'family_type': fields.Raw,
}

job_fields = {
    'id': fields.Raw,
    'state': fields.Raw,
    'created_at': FieldDate,
    'updated_at': FieldDate,
    'data_sets': fields.List(fields.Nested(dataset_field)),
    'instance': fields.Nested(instance_fields)
}

jobs_fields = {
    'jobs': fields.List(fields.Nested(job_fields))
}

one_job_fields = {
    'job': fields.Nested(job_fields)
}

poi_types_fields = {
    'poi_types': fields.List(fields.Nested({
        'uri': fields.Raw,
        'name': fields.Raw,
    }))
}

traveler_profile = {
    'traveler_type': fields.String,
    'walking_speed': fields.Raw,
    'bike_speed': fields.Raw,
    'bss_speed': fields.Raw,
    'car_speed': fields.Raw,
    'wheelchair': fields.Boolean,
    'max_walking_duration_to_pt': fields.Raw,
    'max_bike_duration_to_pt': fields.Raw,
    'max_bss_duration_to_pt': fields.Raw,
    'max_car_duration_to_pt': fields.Raw,
    'first_section_mode': fields.List(fields.String),
    'last_section_mode': fields.List(fields.String),
    'error': fields.String,
}

autocomplete_parameter_fields = {
    'id': fields.Raw,
    'name': fields.Raw,
    'created_at': FieldDate,
    'updated_at': FieldDate,
    'street': fields.Raw,
    'address': fields.Raw,
    'poi': fields.Raw,
    'admin': fields.Raw,
    'admin_level': fields.List(fields.Integer),
}

error_fields = {
    'error': fields.Nested({'message': fields.String})
}
