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
from jormungandr.interfaces.v1.serializer.base import ForwardSerializer
import serpy


class _ParametersSerializer(serpy.Serializer):
    bike_speed = serpy.FloatField()
    bss_provider = serpy.IntField()
    bss_speed = serpy.FloatField()
    car_speed = serpy.FloatField()
    factor_too_long_journey = serpy.IntField()
    journey_order = serpy.StrField()
    max_additional_connections = serpy.IntField()
    max_bike_duration_to_pt = serpy.IntField()
    max_bss_duration_to_pt = serpy.IntField()
    max_car_duration_to_pt = serpy.IntField()
    max_duration = serpy.IntField()
    max_duration_criteria = serpy.StrField()
    max_duration_fallback_mode = serpy.StrField()
    max_nb_transfers = serpy.IntField()
    max_walking_duration_to_pt = serpy.IntField()
    min_bike = serpy.IntField()
    min_bss = serpy.IntField()
    min_car = serpy.IntField()
    min_duration_too_long_journey = serpy.IntField()
    min_tc_with_bike = serpy.IntField()
    min_tc_with_bss = serpy.IntField()
    min_tc_with_car = serpy.IntField()
    night_bus_filter_base_factor = serpy.IntField()
    night_bus_filter_max_factor = serpy.FloatField()
    priority = serpy.IntField()
    scenario = serpy.StrField(attr=str('_scenario_name'))
    successive_physical_mode_to_limit_id = serpy.StrField()
    walking_speed = serpy.FloatField()
    walking_transfer_penalty = serpy.IntField()


class _TravelerProfilesSerializer(serpy.Serializer):
    bike_speed = serpy.FloatField()
    bss_speed = serpy.FloatField()
    car_speed = serpy.FloatField()
    first_section_mode = ForwardSerializer(many=True)
    is_from_db = serpy.BoolField()
    last_section_mode = ForwardSerializer(many=True)
    max_bike_duration_to_pt = serpy.IntField()
    max_bss_duration_to_pt = serpy.IntField()
    max_car_duration_to_pt = serpy.IntField()
    max_walking_duration_to_pt = serpy.IntField()
    traveler_type = serpy.StrField()
    walking_speed = serpy.FloatField()
    wheelchair = serpy.BoolField()


class StatusSerializer(serpy.DictSerializer):
    data_version = serpy.IntField()
    dataset_created_at = serpy.StrField()
    end_production_date = serpy.StrField()
    is_connected_to_rabbitmq = serpy.BoolField()
    is_open_data = serpy.IntField()
    is_open_service = serpy.BoolField()
    is_realtime_loaded = serpy.BoolField()
    kraken_version = serpy.BoolField(attr=str("navitia_version"))
    last_load_at = serpy.StrField()
    last_load_status = serpy.BoolField()
    last_rt_data_loaded = serpy.StrField()
    nb_threads = serpy.IntField()
    parameters = _ParametersSerializer()
    publication_date = serpy.StrField()
    realtime_contributors = serpy.MethodField()
    realtime_proxies = ForwardSerializer(many=True)
    start_production_date = serpy.StrField()
    status = serpy.StrField()
    traveler_profiles = _TravelerProfilesSerializer(many=True)

    def get_realtime_contributors(self, obj):
        # so far, serpy cannot serialize an optional attr
        # this has to be done manually
        rt_contributors = obj.get('rt_contributors')
        if rt_contributors:
            return rt_contributors
        return []
