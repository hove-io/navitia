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

from ...scripts import qualifier
import navitiacommon.response_pb2 as response_pb2
from nose.tools import eq_
from utils import to_time_stamp


#journey.arrival_date_time
#journey.duration
#journey.nb_transfers
#journeys.sections[i].type
#journeys.sections[i].duration
#journey.sections
def qualifier_one_direct_test():
    journeys = []
    journey_direct = response_pb2.Journey()
    journey_direct.arrival_date_time = to_time_stamp("20131107T151200")
    journey_direct.duration = 25 * 60
    journey_direct.nb_transfers = 0
    journey_direct.sections.add()
    journey_direct.sections.add()

    journey_direct.sections[0].type = response_pb2.STREET_NETWORK
    journey_direct.sections[0].duration = 2 * 60

    journey_direct.sections[1].type = response_pb2.STREET_NETWORK
    journey_direct.sections[1].duration = 4 * 60
    journeys.append(journey_direct)

    qualifier.qualifier_one(journeys, "departure")

    assert("non_pt" in journey_direct.type)


# Test with 5 journeys
# a standard with 3 tranfers and 60mn trip
# the fastest with 2 transfers and 62 mn trip
# the healthiest with 1 transfert, 65 mn trip and more walk
# the most confortable with 1 transfert and 80mn trip and less walk
def qualifier_two_test():
    journeys = []

    journey_standard = response_pb2.Journey()
    journey_standard.type = "none"
    journey_standard.arrival_date_time = to_time_stamp("20131107T150000")
    journey_standard.duration = 60 * 60
    journey_standard.nb_transfers = 3
    journey_standard.sections.add()
    journey_standard.sections.add()
    journey_standard.sections.add()
    journey_standard.sections.add()

    journey_standard.sections[0].type = response_pb2.STREET_NETWORK
    journey_standard.sections[0].duration = 10 * 60

    journey_standard.sections[-1].type = response_pb2.STREET_NETWORK
    journey_standard.sections[-1].duration = 10 * 60
    journeys.append(journey_standard)

    journey_rapid = response_pb2.Journey()
    journey_rapid.arrival_date_time = to_time_stamp("20131107T150500")
    journey_rapid.duration = 62 * 60
    journey_rapid.nb_transfers = 2
    journey_rapid.sections.add()
    journey_rapid.sections.add()
    journey_rapid.sections.add()

    journey_rapid.sections[0].type = response_pb2.STREET_NETWORK
    journey_rapid.sections[0].duration = 10 * 60

    journey_rapid.sections[-1].type = response_pb2.STREET_NETWORK
    journey_rapid.sections[-1].duration = 10 * 60
    journeys.append(journey_rapid)

    journey_health = response_pb2.Journey()
    journey_health.arrival_date_time = to_time_stamp("20131107T151000")
    journey_health.duration = 70 * 60
    journey_health.nb_transfers = 1
    journey_health.sections.add()
    journey_health.sections.add()
    journey_health.sections.add()

    journey_health.sections[0].type = response_pb2.STREET_NETWORK
    journey_health.sections[0].duration = 15 * 60

    journey_health.sections[1].type = response_pb2.TRANSFER
    journey_health.sections[1].duration = 10 * 60

    journey_health.sections[-1].type = response_pb2.STREET_NETWORK
    journey_health.sections[-1].duration = 10 * 60
    journeys.append(journey_health)

    journey_confort = response_pb2.Journey()
    journey_confort.arrival_date_time = to_time_stamp("20131107T152000")
    journey_confort.duration = 80 * 60
    journey_confort.nb_transfers = 1
    journey_confort.sections.add()
    journey_confort.sections.add()
    journey_confort.sections.add()

    journey_confort.sections[0].type = response_pb2.STREET_NETWORK
    journey_confort.sections[0].duration = 5 * 60

    journey_confort.sections[-1].type = response_pb2.STREET_NETWORK
    journey_confort.sections[-1].duration = 5 * 60
    journeys.append(journey_confort)

    qualifier.qualifier_one(journeys, "departure")

    eq_(journey_standard.type, "fastest")  # the standard should be the fastest
    eq_(journey_rapid.type, "rapid")

    #TODO! refacto this test with custom rules not to depends on changing business rules
#    eq_(journey_confort.type, "comfort")
#    eq_(journey_health.type, "healthy")


def has_car_test():
    journey = response_pb2.Journey()
    journey.sections.add()
    section = journey.sections[0]
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    assert(qualifier.has_car(journey))

    foot_journey = response_pb2.Journey()
    foot_journey.sections.add()
    foot_journey.sections.add()
    foot_journey.sections.add()
    foot_journey.sections[0].street_network.mode = response_pb2.Walking
    foot_journey.sections[1].street_network.mode = response_pb2.Bike
    foot_journey.sections[2].street_network.mode = response_pb2.Bss
    assert(not qualifier.has_car(foot_journey))

    foot_journey.sections.add()
    foot_journey.sections[3].type = response_pb2.STREET_NETWORK
    foot_journey.sections[3].street_network.mode = response_pb2.Car
    assert(qualifier.has_car(foot_journey))


def standard_choice_test():
    journeys = []
    #the first is the worst one
    journey_worst = response_pb2.Journey()
    journey_worst.arrival_date_time = to_time_stamp("20131107T161200")
    journey_worst.sections.add()

    journey_worst.sections[0].type = response_pb2.STREET_NETWORK
    journey_worst.sections[0].street_network.mode = response_pb2.Car

    journeys.append(journey_worst)

    # arrive later but no car
    journey_not_good = response_pb2.Journey()
    journey_not_good.arrival_date_time = to_time_stamp("20131107T171200")
    journey_not_good.sections.add()

    journey_not_good.sections[0].type = response_pb2.STREET_NETWORK
    journey_not_good.sections[0].street_network.mode = response_pb2.Bike

    journeys.append(journey_not_good)

    #this is the standard
    journey_1 = response_pb2.Journey()
    journey_1.arrival_date_time = to_time_stamp("20131107T151200")
    journey_1.sections.add()

    journey_1.sections[0].type = response_pb2.STREET_NETWORK
    journey_1.sections[0].street_network.mode = response_pb2.Bike

    journeys.append(journey_1)

    # a better journey, but using car
    journey_2 = response_pb2.Journey()
    journey_2.arrival_date_time = to_time_stamp("20131107T151000")
    journey_2.sections.add()

    journey_2.sections[0].type = response_pb2.STREET_NETWORK
    journey_2.sections[0].street_network.mode = response_pb2.Car

    journeys.append(journey_2)

    standard = qualifier.choose_standard(journeys, qualifier.arrival_crit)

    print qualifier.has_car(standard)
    print "standard ", standard.arrival_date_time
    eq_(standard, journey_1)


def tranfers_cri_test():
    journeys = []

    dates = ["20131107T100000", "20131107T150000", "20131107T050000",
             "20131107T100000", "20131107T150000", "20131107T050000"]
    transfers = [4, 3, 8, 1, 1, 2]
    for i in range(6):
        journey = response_pb2.Journey()
        journey.nb_transfers = transfers[i]
        journey.arrival_date_time = to_time_stamp(dates[i])

        journeys.append(journey)

    best = qualifier.min_from_criteria(journeys, [qualifier.transfers_crit,
                                                  qualifier.arrival_crit])

    #the transfert criterion is first, and then if 2 journeys have
    #the same nb_transfers, we compare the dates
    eq_(best.nb_transfers, 1)
    eq_(best.arrival_date_time, to_time_stamp("20131107T100000"))
