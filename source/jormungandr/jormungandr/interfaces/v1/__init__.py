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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

__all__ = [
    'Uri',
    'Coverage',
    'Journeys',
    'Places',
    'ResourceUri',
    'Schedules',
    'Disruptions',
    'Calendars',
    'Ptobjects',
    'JSONSchema',
]


def add_common_status(response, instance):
    response['status']["is_open_data"] = instance.is_open_data
    response['status']["is_open_service"] = instance.is_free
    response['status']['realtime_proxies'] = []
    for realtime_proxy in instance.realtime_proxy_manager.realtime_proxies.values():
        response['status']['realtime_proxies'].append(realtime_proxy.status())

    response['status']['street_networks'] = []
    for sn in instance.get_all_street_networks():
        response['status']['street_networks'].append(sn.status())

    response['status']['ridesharing_services'] = []
    for rss in instance.get_all_ridesharing_services():
        response['status']['ridesharing_services'].append(rss.status())

    response['status']['equipment_providers_services'] = {}
    response['status']['equipment_providers_services'][
        'equipment_providers_keys'
    ] = instance.equipment_provider_manager.providers_keys
    response['status']['equipment_providers_services'][
        'equipment_providers'
    ] = instance.equipment_provider_manager.status()

    response['status']['autocomplete'] = instance.autocomplete.status()
