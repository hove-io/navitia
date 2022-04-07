# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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

from jormungandr.interfaces.v1.serializer.base import PbNestedSerializer
from jormungandr.interfaces.v1.serializer import pt


class LineReportSerializer(PbNestedSerializer):
    line = pt.LineSerializer()
    pt_objects = pt.PtObjectSerializer(many=True, display_none=True)


class EquipmentReportSerializer(PbNestedSerializer):
    line = pt.LineSerializer()
    stop_area_equipments = pt.StopAreaEquipmentsSerializer(many=True)


class TrafficReportSerializer(PbNestedSerializer):
    network = pt.NetworkSerializer()
    lines = pt.LineSerializer(many=True)
    stop_areas = pt.StopAreaSerializer(many=True)
    vehicle_journeys = pt.VehicleJourneySerializer(many=True)


class VehiclePositionsSerializer(PbNestedSerializer):
    line = pt.LineSerializer()
    vehicle_journey_positions = pt.VehicleJourneyPositionsSerializer(many=True)
