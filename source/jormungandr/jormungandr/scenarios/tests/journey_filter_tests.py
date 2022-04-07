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
