# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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
from jormungandr.interfaces.v1.serializer.jsonschema import MethodField, FloatField, IntField, StrField, BoolField
from jormungandr.interfaces.v1.serializer.pt import StringListField
import serpy


class ParametersSerializer(serpy.Serializer):
    bike_speed = FloatField()
    bss_provider = BoolField()
    bss_speed = FloatField()
    car_speed = FloatField()
    factor_too_long_journey = IntField()
    journey_order = StrField()
    max_additional_connections = IntField()
    max_bike_duration_to_pt = IntField()
    max_bss_duration_to_pt = IntField()
    max_car_duration_to_pt = IntField()
    max_duration = IntField()
    max_duration_criteria = StrField()
    max_duration_fallback_mode = StrField()
    max_nb_transfers = IntField()
    max_walking_duration_to_pt = IntField()
    min_bike = IntField()
    min_bss = IntField()
    min_car = IntField()
    min_duration_too_long_journey = IntField()
    min_tc_with_bike = IntField()
    min_tc_with_bss = IntField()
    min_tc_with_car = IntField()
    night_bus_filter_base_factor = IntField()
    night_bus_filter_max_factor = FloatField()
    priority = IntField()
    scenario = StrField(attr=str('_scenario_name'))
    successive_physical_mode_to_limit_id = StrField()
    walking_speed = FloatField()
    walking_transfer_penalty = IntField()


class TravelerProfilesSerializer(serpy.Serializer):
    bike_speed = FloatField()
    bss_speed = FloatField()
    car_speed = FloatField()
    first_section_mode = StringListField()
    is_from_db = BoolField()
    last_section_mode = StringListField(display_none=True)
    max_bike_duration_to_pt = IntField()
    max_bss_duration_to_pt = IntField()
    max_car_duration_to_pt = IntField()
    max_walking_duration_to_pt = IntField()
    traveler_type = StrField()
    walking_speed = FloatField()
    wheelchair = BoolField()


class StatusSerializer(serpy.DictSerializer):
    data_version = IntField()
    dataset_created_at = StrField()
    end_production_date = StrField()
    is_connected_to_rabbitmq = BoolField()
    is_open_data = BoolField()
    is_open_service = BoolField()
    is_realtime_loaded = BoolField()
    kraken_version = StrField(attr=str("navitia_version"))
    last_load_at = StrField()
    last_load_status = BoolField()
    last_rt_data_loaded = StrField()
    nb_threads = IntField()
    parameters = ParametersSerializer()
    publication_date = StrField()
    realtime_contributors = MethodField(schema_type=str, many=True, display_none=True)
    realtime_proxies = StringListField(display_none=True)
    start_production_date = StrField()
    status = StrField()
    traveler_profiles = TravelerProfilesSerializer(many=True)

    def get_realtime_contributors(self, obj):
        # so far, serpy cannot serialize an optional attr
        # this has to be done manually
        return obj.get('rt_contributors', [])

