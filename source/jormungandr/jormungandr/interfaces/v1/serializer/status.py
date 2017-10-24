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
from jormungandr.interfaces.v1.serializer.jsonschema import Field, MethodField
from jormungandr.interfaces.v1.serializer.pt import StringListField
import serpy


class ParametersSerializer(serpy.Serializer):
    bike_speed = Field(schema_type=float)
    bss_provider = Field(schema_type=bool)
    bss_speed = Field(schema_type=float)
    car_speed = Field(schema_type=float)
    factor_too_long_journey = Field(schema_type=int)
    journey_order = Field(schema_type=str)
    max_additional_connections = Field(schema_type=int)
    max_bike_duration_to_pt = Field(schema_type=int)
    max_bss_duration_to_pt = Field(schema_type=int)
    max_car_duration_to_pt = Field(schema_type=int)
    max_duration = Field(schema_type=int)
    max_duration_criteria = Field(schema_type=str)
    max_duration_fallback_mode = Field(schema_type=str)
    max_nb_transfers = Field(schema_type=int)
    max_walking_duration_to_pt = Field(schema_type=int)
    min_bike = Field(schema_type=int)
    min_bss = Field(schema_type=int)
    min_car = Field(schema_type=int)
    min_duration_too_long_journey = Field(schema_type=int)
    min_tc_with_bike = Field(schema_type=int)
    min_tc_with_bss = Field(schema_type=int)
    min_tc_with_car = Field(schema_type=int)
    night_bus_filter_base_factor = Field(schema_type=int)
    night_bus_filter_max_factor = Field(schema_type=float)
    priority = Field(schema_type=int)
    scenario = Field(schema_type=str, attr=str('_scenario_name'))
    successive_physical_mode_to_limit_id = Field(schema_type=str)
    walking_speed = Field(schema_type=float)
    walking_transfer_penalty = Field(schema_type=int)


class TravelerProfilesSerializer(serpy.Serializer):
    bike_speed = Field(schema_type=float)
    bss_speed = Field(schema_type=float)
    car_speed = Field(schema_type=float)
    first_section_mode = StringListField()
    is_from_db = Field(schema_type=bool)
    last_section_mode = StringListField()
    max_bike_duration_to_pt = Field(schema_type=int)
    max_bss_duration_to_pt = Field(schema_type=int)
    max_car_duration_to_pt = Field(schema_type=int)
    max_walking_duration_to_pt = Field(schema_type=int)
    traveler_type = Field(schema_type=str)
    walking_speed = Field(schema_type=float)
    wheelchair = Field(schema_type=bool)

class AutocompleteSerializer(serpy.DictSerializer):
    class_ = Field(schema_type=str, label='class', attr='class')
    timeout = MethodField(schema_type=float, display_none=False)

    def get_timeout(self, obj):
        return obj.get('timeout', None)

class CircuitBreakerSerializer(serpy.DictSerializer):
    current_state = Field(schema_type=str, display_none=True)
    fail_counter = Field(schema_type=int, display_none=True)
    reset_timeout = Field(schema_type=int, display_none=True)

class StreetNetworkSerializer(serpy.DictSerializer):
    class_ = Field(schema_type=str, label='class', attr='class')
    id = Field(schema_type=str)
    modes = StringListField(display_none=True)
    timeout = MethodField(schema_type=float, display_none=False)
    circuit_breaker = MethodField(schema_type=float, display_none=False)

    def get_timeout(self, obj):
        return obj.get('timeout', None)

    def get_circuit_breaker(self, obj):
        o = obj.get('circuit_breaker', None)
        return CircuitBreakerSerializer(o, display_none=False).data if o else None


class StatusSerializer(serpy.DictSerializer):
    data_version = Field(schema_type=int)
    dataset_created_at = Field(schema_type=str)
    end_production_date = Field(schema_type=str)
    is_connected_to_rabbitmq = Field(schema_type=bool)
    is_open_data = Field(schema_type=bool)
    is_open_service = Field(schema_type=bool)
    is_realtime_loaded = Field(schema_type=bool)
    kraken_version = Field(schema_type=str, attr=str("navitia_version"))
    last_load_at = Field(schema_type=str)
    last_load_status = Field(schema_type=bool)
    last_rt_data_loaded = Field(schema_type=str)
    nb_threads = Field(schema_type=int)
    parameters = ParametersSerializer()
    publication_date = Field(schema_type=str)
    realtime_contributors = MethodField(schema_type=str, many=True, display_none=True)
    realtime_proxies = StringListField(display_none=True)
    autocomplete = AutocompleteSerializer(display_none=True)
    street_networks = StreetNetworkSerializer(many=True, display_none=True)
    start_production_date = Field(schema_type=str)
    status = Field(schema_type=str)
    traveler_profiles = TravelerProfilesSerializer(many=True)

    def get_realtime_contributors(self, obj):
        # so far, serpy cannot serialize an optional attr
        # this has to be done manually
        return obj.get('rt_contributors', [])

