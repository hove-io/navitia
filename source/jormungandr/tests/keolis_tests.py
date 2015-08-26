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

from nose.tools import eq_

from jormungandr.scenarios import keolis
from jormungandr.travelers_profile import default_traveler_profiles
import navitiacommon.response_pb2 as response_pb2
from jormungandr.utils import str_to_time_stamp


def one_best_test():
    def check(profile, type):
        scenario = keolis.Scenario()
        response = response_pb2.Response()
        journey_direct = response.journeys.add()

        journey_direct.type = "best"

        #we set the destination else the filter will not be applicated
        req = {'debug': False, 'destination': 'foo'}
        scenario._qualification(req, response, default_traveler_profiles[profile])
        eq_(type, journey_direct.type)

    for profile in default_traveler_profiles:
        yield (check, profile, 'rapid')


def best_and_fallback_test():
    def check(profile, type_direct, type_fallback_walking, type_fallback_bike, nb):
        scenario = keolis.Scenario()

        response = response_pb2.Response()
        journey_direct = response.journeys.add()
        journey_fallback_walking = response.journeys.add()
        journey_fallback_bike = response.journeys.add()

        journey_direct.type = "best"
        journey_fallback_walking.type = "less_fallback_walk"
        journey_fallback_bike.type = "less_fallback_bss"
        #we set the destination else the filter will not be applicated
        req = {'debug': False, 'destination': 'foo'}
        scenario._qualification(req, response, default_traveler_profiles[profile])
        eq_(type_direct, journey_direct.type)
        eq_(type_fallback_walking, journey_fallback_walking.type)
        eq_(type_fallback_bike, journey_fallback_bike.type)
        eq_(len(response.journeys), nb)

    for profile in default_traveler_profiles:
        yield (check, profile, 'rapid', 'comfort', '', 2)

def car_test():
    def check(profile, type_car, nb):
        scenario = keolis.Scenario()

        response = response_pb2.Response()
        journey_car = response.journeys.add()

        journey_car.type = "car"
        #we set the destination else the filter will not be applicated
        req = {'debug': False, 'destination': 'foo'}
        scenario._qualification(req, response, default_traveler_profiles[profile])
        eq_(type_car, journey_car.type)
        eq_(len(response.journeys), nb)

    tests = [('standard', '', 0), ('slow_walker', '', 0),
             ('fast_walker', '', 0), ('wheelchair', '', 0), ('luggage', '', 0),
             # Temporary profiles
             ('cyclist', '', 0), ('motorist', 'comfort', 1),
            ]
    for test in tests:
        yield (check, test[0], test[1], test[2])

def best_and_bike_test():
    def check(profile, type_direct, type_fallback_bike, nb):
        scenario = keolis.Scenario()

        response = response_pb2.Response()
        journey_direct = response.journeys.add()
        journey_fallback_bike = response.journeys.add()

        journey_direct.type = "best"
        journey_fallback_bike.type = "less_fallback_bike"
        #we set the destination else the filter will not be applicated
        req = {'debug': False, 'destination': 'foo'}
        scenario._qualification(req, response, default_traveler_profiles[profile])
        eq_(type_direct, journey_direct.type)
        eq_(type_fallback_bike, journey_fallback_bike.type)
        eq_(len(response.journeys), nb)

    tests = [('standard', 'rapid', '', 1), ('slow_walker', 'rapid', '', 1),
             ('fast_walker', 'rapid', '', 1), ('wheelchair', 'rapid', '', 1),
             ('luggage', 'rapid', '', 1),
             # Temporary profiles
             ('cyclist', 'rapid', 'comfort', 2), ('motorist', 'rapid', '', 1),
             ]
    for test in tests:
        yield (check, test[0], test[1], test[2], test[3])
