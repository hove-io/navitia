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
import datetime
import navitiacommon.response_pb2 as response_pb2
from jormungandr.utils import str_to_time_stamp, date_to_timestamp


class ResponseBuilder(object):
    """
    Object to build protobuf in tests
    """

    def __init__(self, default_date=datetime.date(2016, 3, 25)):
        self.response = response_pb2.Response()
        self._journeys = {}
        self._default_date = default_date

    def __to_timestamp(self, str):
        try:  # first we try with only a time
            time = datetime.datetime.strptime(str, "T%H%M")
            dt = datetime.datetime.combine(self._default_date, time.time())
        except ValueError:  # else we try with the date
            dt = datetime.datetime.strptime(str, "%Y%m%dT%H%M%S")

        return date_to_timestamp(dt)

    def get_journey(self, uri):
        return self._journeys[uri]

    def get_journeys(self):
        return self.response.journeys

    @staticmethod
    def __init_section(pb_section, **kwargs):
        """
        Fill the protobuf section with the params

        The section can be:
        'PT': for a Public Transport section
        'Walking', 'Bike', 'Bss', 'Car': for a street network section

        All other args are forwarded to the protobuf section
        """
        mode = kwargs.pop('mode', None)
        if mode is not None:
            assert mode in ('Walking', 'Bike', 'Bss', 'Car')
            if not pb_section.HasField('type'):
                pb_section.type = response_pb2.STREET_NETWORK
            pb_section.street_network.mode = getattr(response_pb2, mode)

        _type = kwargs.pop('type', None)
        if _type == 'PT':
            pb_section.type = response_pb2.PUBLIC_TRANSPORT
        elif _type is not None:
            pb_section.type = getattr(response_pb2, _type)
        for k, v in kwargs.items():
            setattr(pb_section, k, v)

    def journey(self, uri=None, sections=[], **kwargs):
        j = self.response.journeys.add()

        if uri is not None:  # a name can be provided be find easily the journey
            self._journeys[uri] = j

        for section_params in sections:
            self.__init_section(j.sections.add(), **section_params)

        arrival = kwargs.pop('arrival', None)
        if arrival is not None:
            j.arrival_date_time = self.__to_timestamp(arrival)
        departure = kwargs.pop('departure', None)
        if departure is not None:
            j.departure_date_time = self.__to_timestamp(departure)
        for k, v in kwargs.items():
            setattr(j, k, v)
        self.__compute_values(j)
        return self

    def __compute_values(self, journey):
        """ Compute some values on the journey """
        if not journey.HasField('duration'):
            journey.duration = sum(section.duration for section in journey.sections)
        if not journey.HasField('arrival_date_time') and journey.HasField('departure_date_time'):
            journey.arrival_date_time = journey.departure_date_time + journey.duration
        elif not journey.HasField('departure_date_time') and journey.HasField('arrival_date_time'):
            journey.departure_date_time = journey.arrival_date_time - journey.duration
