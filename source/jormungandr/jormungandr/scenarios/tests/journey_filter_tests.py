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
from jormungandr.scenarios import journey_filter
import navitiacommon.response_pb2 as response_pb2


def remove_excess_terminus_without_item_test():
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.STREET_NETWORK
    journey.sections[0].duration = 796

    journey_filter.remove_excess_terminus(response)
    assert len(response.terminus) == 0


def remove_excess_terminus_without_uri_in_display_information_test():
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.STREET_NETWORK
    journey.sections[0].duration = 796

    terminus = response.terminus.add()
    terminus.uri = "aaa"
    assert len(response.terminus) == 1

    journey_filter.remove_excess_terminus(response)
    assert len(response.terminus) == 0


def remove_excess_terminus_with_uri_in_display_information_test():
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[0].duration = 796
    journey.sections[0].pt_display_informations.uris.stop_area = "aaa"

    terminus = response.terminus.add()
    terminus.uri = "aaa"
    assert len(response.terminus) == 1

    journey_filter.remove_excess_terminus(response)
    assert len(response.terminus) == 1


def remove_excess_terminus_with_2items_uri_in_display_information_test():
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[0].duration = 796
    journey.sections[0].pt_display_informations.uris.stop_area = "aaa"

    terminus = response.terminus.add()
    terminus.uri = "aaa"
    terminus = response.terminus.add()
    terminus.uri = "bbb"
    assert len(response.terminus) == 2

    journey_filter.remove_excess_terminus(response)
    assert len(response.terminus) == 1


def filter_odt_journeys_clockwise_test():
    request = {"clockwise": True}

    response = response_pb2.Response()
    # a public transport journey
    pt_journey = response.journeys.add()
    pt_journey.arrival_date_time = 10
    pt_section = pt_journey.sections.add()
    pt_section.type = response_pb2.PUBLIC_TRANSPORT

    # an On Demand Transport journey
    # that arrives after the public transport journey
    odt_journey = response.journeys.add()
    odt_section = odt_journey.sections.add()
    odt_section.additional_informations.append(response_pb2.ODT_WITH_ZONE)
    odt_journey.arrival_date_time = 11

    journey_filter.filter_odt_journeys(response.journeys, request)

    assert odt_journey.tags[0] == "to_delete"


def filter_odt_journeys_counter_clockwise_test():
    request = {"clockwise": False}

    response = response_pb2.Response()
    # a public transport journey
    pt_journey = response.journeys.add()
    pt_journey.departure_date_time = 10
    pt_section = pt_journey.sections.add()
    pt_section.type = response_pb2.PUBLIC_TRANSPORT

    # an On Demand Transport journey
    # that depart before the public transport journey
    odt_journey = response.journeys.add()
    odt_section = odt_journey.sections.add()
    odt_section.additional_informations.append(response_pb2.ODT_WITH_ZONE)
    odt_journey.departure_date_time = 9

    journey_filter.filter_odt_journeys(response.journeys, request)

    assert odt_journey.tags[0] == "to_delete"
