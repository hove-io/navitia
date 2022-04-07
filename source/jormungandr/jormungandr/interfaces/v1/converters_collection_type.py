# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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

from __future__ import absolute_import, print_function, unicode_literals, division

collections_to_resource_type = {
    "stop_points": "stop_point",
    "routes": "route",
    "networks": "network",
    "commercial_modes": "commercial_mode",
    "physical_modes": "physical_mode",
    "companies": "company",
    "stop_areas": "stop_area",
    "lines": "line",
    "line_groups": "line_group",
    "addresses": "address",
    "coords": "coord",
    "coord": "coord",
    "journey_pattern_points": "journey_pattern_point",
    "journey_patterns": "journey_pattern",
    "pois": "poi",
    "poi_types": "poi_type",
    "connections": "connection",
    "vehicle_journeys": "vehicle_journey",
    "disruptions": "disruption",
    "trips": "trip",
    "contributors": "contributor",
    "datasets": "dataset",
}

resource_type_to_collection = dict(
    (resource_type, collection) for (collection, resource_type) in collections_to_resource_type.items()
)
