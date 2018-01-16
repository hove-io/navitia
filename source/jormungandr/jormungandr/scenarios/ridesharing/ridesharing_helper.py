# Copyright (c) 2001-2018, Canal TP and/or its affiliates. All rights reserved.
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
import math
from jormungandr.scenarios.helper_classes.helper_utils import crowfly_distance_between
from jormungandr.utils import get_pt_object_coord
from navitiacommon import response_pb2


def build_ridesharing_crowfly_journey(instance, origin, destination, period_extremity):
    ridesharing_journey = response_pb2.Journey()

    # manage section
    ridesharing_section = ridesharing_journey.sections.add()
    ridesharing_section.type = response_pb2.CROW_FLY
    ridesharing_section.street_network.mode = response_pb2.Ridesharing
    ridesharing_section.origin.CopyFrom(instance.georef.place(origin))
    ridesharing_section.destination.CopyFrom(instance.georef.place(destination))

    orig_coord = get_pt_object_coord(ridesharing_section.origin)
    dest_coord = get_pt_object_coord(ridesharing_section.destination)
    start = ridesharing_section.shape.add()
    start.lon = orig_coord.lon
    start.lat = orig_coord.lat
    end = ridesharing_section.shape.add()
    end.lon = dest_coord.lon
    end.lat = dest_coord.lat

    distance = crowfly_distance_between(orig_coord, dest_coord)
    ridesharing_section.length = int(distance)
    # manhattan + 15min
    ridesharing_section.duration = int(distance / (instance.car_speed / math.sqrt(2))) + 15*60
    if period_extremity.represents_start:
        ridesharing_section.begin_date_time = period_extremity.datetime
        ridesharing_section.end_date_time = period_extremity.datetime + ridesharing_section.duration
    else:
        ridesharing_section.begin_date_time = period_extremity.datetime - ridesharing_section.duration
        ridesharing_section.end_date_time = period_extremity.datetime

    # report section values into journey
    ridesharing_journey.tags.append('ridesharing')
    ridesharing_journey.distances.ridesharing = ridesharing_section.length
    ridesharing_journey.durations.ridesharing = ridesharing_section.duration
    ridesharing_journey.duration = ridesharing_section.duration
    ridesharing_journey.durations.total = ridesharing_section.duration
    ridesharing_journey.requested_date_time = period_extremity.datetime
    ridesharing_journey.departure_date_time = ridesharing_section.begin_date_time
    ridesharing_journey.arrival_date_time = ridesharing_section.end_date_time

    return ridesharing_journey
