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
from jormungandr.scenarios import qualifier
from jormungandr.scenarios.new_default import is_reliable_journey
import navitiacommon.response_pb2 as response_pb2
from jormungandr.utils import str_to_time_stamp
from six.moves import range


def has_car_test():
    journey = response_pb2.Journey()
    journey.sections.add()
    section = journey.sections[0]
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    assert qualifier.has_car(journey)

    foot_journey = response_pb2.Journey()
    foot_journey.sections.add()
    foot_journey.sections.add()
    foot_journey.sections.add()
    foot_journey.sections[0].street_network.mode = response_pb2.Walking
    foot_journey.sections[1].street_network.mode = response_pb2.Bike
    foot_journey.sections[2].street_network.mode = response_pb2.Bss
    assert not qualifier.has_car(foot_journey)

    foot_journey.sections.add()
    foot_journey.sections[3].type = response_pb2.STREET_NETWORK
    foot_journey.sections[3].street_network.mode = response_pb2.Car
    assert qualifier.has_car(foot_journey)


def standard_choice_test():
    journeys = []
    # the first is the worst one
    journey_worst = response_pb2.Journey()
    journey_worst.arrival_date_time = str_to_time_stamp("20131107T161200")
    journey_worst.sections.add()

    journey_worst.sections[0].type = response_pb2.STREET_NETWORK
    journey_worst.sections[0].street_network.mode = response_pb2.Car

    journeys.append(journey_worst)

    # arrive later but no car
    journey_not_good = response_pb2.Journey()
    journey_not_good.arrival_date_time = str_to_time_stamp("20131107T171200")
    journey_not_good.sections.add()

    journey_not_good.sections[0].type = response_pb2.STREET_NETWORK
    journey_not_good.sections[0].street_network.mode = response_pb2.Bike

    journeys.append(journey_not_good)

    # this is the standard
    journey_1 = response_pb2.Journey()
    journey_1.arrival_date_time = str_to_time_stamp("20131107T151200")
    journey_1.sections.add()

    journey_1.sections[0].type = response_pb2.STREET_NETWORK
    journey_1.sections[0].street_network.mode = response_pb2.Bike

    journeys.append(journey_1)

    # a better journey, but using car
    journey_2 = response_pb2.Journey()
    journey_2.arrival_date_time = str_to_time_stamp("20131107T151000")
    journey_2.sections.add()

    journey_2.sections[0].type = response_pb2.STREET_NETWORK
    journey_2.sections[0].street_network.mode = response_pb2.Car

    journeys.append(journey_2)

    standard = qualifier.choose_standard(journeys, qualifier.arrival_crit)

    print(qualifier.has_car(standard))
    print("standard ", standard.arrival_date_time)
    assert standard == journey_1


def standard_choice_with_pt_test():
    journeys = []
    # the first is the worst one
    journey_worst = response_pb2.Journey()
    journey_worst.arrival_date_time = str_to_time_stamp("20131107T161200")
    journey_worst.sections.add()

    journey_worst.sections[0].type = response_pb2.STREET_NETWORK
    journey_worst.sections[0].street_network.mode = response_pb2.Car

    journeys.append(journey_worst)

    # arrive later but no car
    journey_not_good = response_pb2.Journey()
    journey_not_good.arrival_date_time = str_to_time_stamp("20131107T171200")
    journey_not_good.sections.add()

    journey_not_good.sections[0].type = response_pb2.STREET_NETWORK
    journey_not_good.sections[0].street_network.mode = response_pb2.Bike

    journeys.append(journey_not_good)

    # this is the standard
    journey_1 = response_pb2.Journey()
    journey_1.arrival_date_time = str_to_time_stamp("20131107T201200")
    journey_1.sections.add()
    journey_1.sections[0].type = response_pb2.PUBLIC_TRANSPORT

    journeys.append(journey_1)

    # a better journey, but using car
    journey_2 = response_pb2.Journey()
    journey_2.arrival_date_time = str_to_time_stamp("20131107T151000")
    journey_2.sections.add()

    journey_2.sections[0].type = response_pb2.STREET_NETWORK
    journey_2.sections[0].street_network.mode = response_pb2.Car

    journeys.append(journey_2)

    standard = qualifier.choose_standard(journeys, qualifier.arrival_crit)

    print(qualifier.has_car(standard))
    print("standard ", standard.arrival_date_time)
    assert standard == journey_1


def choose_standard_pt_car():
    journeys = []

    journey1 = response_pb2.Journey()
    journey1.arrival_date_time = str_to_time_stamp("20141120T170000")
    journey1.sections.add()

    journey1.sections[0].type = response_pb2.STREET_NETWORK
    journey1.sections[0].street_ne


def tranfers_cri_test():
    journeys = []

    dates = [
        "20131107T100000",
        "20131107T150000",
        "20131107T050000",
        "20131107T100000",
        "20131107T150000",
        "20131107T050000",
    ]
    transfers = [4, 3, 8, 1, 1, 2]
    for i in range(6):
        journey = response_pb2.Journey()
        journey.nb_transfers = transfers[i]
        journey.arrival_date_time = str_to_time_stamp(dates[i])

        journeys.append(journey)

    best = qualifier.min_from_criteria(journeys, [qualifier.transfers_crit, qualifier.arrival_crit])

    # the transfert criterion is first, and then if 2 journeys have
    # the same nb_transfers, we compare the dates
    assert best.nb_transfers == 1
    assert best.arrival_date_time == str_to_time_stamp("20131107T100000")


def qualifier_nontransport_duration_only_walk_test():
    journey = response_pb2.Journey()
    journey.arrival_date_time = str_to_time_stamp('20140825T113224')
    journey.duration = 2620
    journey.nb_transfers = 1
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.CROW_FLY
    journey.sections[0].duration = 796

    journey.sections[1].type = response_pb2.TRANSFER
    journey.sections[1].duration = 328

    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].duration = 864

    assert qualifier.get_nontransport_duration(journey) == 1988


def qualifier_nontransport_duration_with_tc_test():
    journey = response_pb2.Journey()
    journey.arrival_date_time = str_to_time_stamp('20140825T113224')
    journey.duration = 2620
    journey.nb_transfers = 2
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.CROW_FLY
    journey.sections[0].duration = 796

    journey.sections[1].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[1].duration = 328

    journey.sections[2].type = response_pb2.TRANSFER
    journey.sections[2].duration = 418

    journey.sections[3].type = response_pb2.WAITING
    journey.sections[3].duration = 719

    journey.sections[4].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[4].duration = 175

    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].duration = 864

    assert qualifier.get_nontransport_duration(journey) == 2797


def reliable_journey_test():
    journey = response_pb2.Journey()
    assert is_reliable_journey(journey) == False

    journey.sections.add()
    journey.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[0].uris.physical_mode = "physical_mode:Bus"

    assert is_reliable_journey(journey) == False

    journey.sections[0].uris.physical_mode = "physical_mode:Train"

    assert is_reliable_journey(journey) == True

    journey.sections.add()
    journey.sections[1].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[1].uris.physical_mode = "physical_mode:Bus"

    assert is_reliable_journey(journey) == False

    journey.sections[1].uris.physical_mode = "physical_mode:Train"
    assert is_reliable_journey(journey) == True

    journey.most_serious_disruption_effect = "SIGNIFICANT_DELAYS"

    assert is_reliable_journey(journey) == False


def reliable_journey_with_fallbacks_test():
    journey = response_pb2.Journey()
    assert is_reliable_journey(journey) == False

    journey.sections.add()
    journey.sections[0].street_network.mode = response_pb2.Bike

    # no public transport in the journey, so it should not be tagged as reliable
    assert is_reliable_journey(journey) == False

    journey.sections.add()
    journey.sections[1].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[1].uris.physical_mode = "physical_mode:Bus"

    assert is_reliable_journey(journey) == False

    journey.sections[1].uris.physical_mode = "physical_mode:Train"

    assert is_reliable_journey(journey) == True

    journey.sections.add()
    journey.sections[2].street_network.mode = response_pb2.Car

    assert is_reliable_journey(journey) == False

    journey.sections[2].street_network.mode = response_pb2.Walking

    assert is_reliable_journey(journey) == True

    journey.sections[0].street_network.mode = response_pb2.Car

    assert is_reliable_journey(journey) == False
