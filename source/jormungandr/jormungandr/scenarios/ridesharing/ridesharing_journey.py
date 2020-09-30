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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division


class Gender(object):
    """
    Used as an enum
    """

    UNKNOWN = 0
    MALE = 1
    FEMALE = 2


class Individual(object):
    # https://stackoverflow.com/a/28059785/1614576
    __slots__ = ('alias', 'gender', 'image', 'rate', 'rate_count')

    def __init__(self, alias, gender, image, rate, rate_count):
        self.alias = alias
        self.gender = gender
        self.image = image
        self.rate = rate
        self.rate_count = rate_count


class Place(object):
    __slots__ = ('addr', 'lat', 'lon')

    def __init__(self, addr, lat, lon):
        self.addr = addr
        self.lat = lat
        self.lon = lon


class MetaData(object):
    __slots__ = ('system_id', 'network', 'rating_scale_min', 'rating_scale_max')

    def __init__(self, system_id, network, rating_scale_min, rating_scale_max):
        self.system_id = system_id
        self.network = network
        self.rating_scale_min = rating_scale_min
        self.rating_scale_max = rating_scale_max


class RidesharingJourney(object):
    __slots__ = (
        'metadata',
        'distance',
        'duration',
        'shape',  # a list of type_pb2.GeographicalCoord()
        'ridesharing_ad',
        'pickup_place',
        'dropoff_place',
        'pickup_date_time',
        'dropoff_date_time',
        # driver will be Individual
        'driver',
        'price',
        'currency',  # "centime" (EURO cents) is the preferred currency (price is filled accordingly)
        'total_seats',
        'available_seats',
    )
