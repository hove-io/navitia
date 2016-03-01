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
from __future__ import absolute_import, print_function

from jormungandr.interfaces.v1.Calendars import calendar
from jormungandr.interfaces.v1 import fields
from jormungandr.interfaces.v1.fields import NonNullList, NonNullNested, NonNullProtobufNested, PbField, FirstComment, \
    comment, DisruptionLinks

vehicle_journey = {
    "id": fields.fields.String(attribute="uri"),
    "name": fields.fields.String(),
    "disruptions": DisruptionLinks(),
    "journey_pattern": PbField(fields.journey_pattern),
    "stop_times": NonNullList(NonNullNested(fields.stop_time)),
    "comment": FirstComment(),
    # for compatibility issue we keep a 'comment' field where we output the first comment (TODO v2)
    "comments": NonNullList(NonNullNested(comment)),
    "codes": NonNullList(NonNullNested(fields.code)),
    "validity_pattern": NonNullProtobufNested(fields.validity_pattern),
    "calendars": NonNullList(NonNullNested(calendar)),
    "trip": NonNullProtobufNested(fields.trip),
}
