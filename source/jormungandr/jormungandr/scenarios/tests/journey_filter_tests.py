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
from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr.scenarios import journey_filter as jf
import navitiacommon.response_pb2 as response_pb2


def remove_excess_tickets_or_ticket_links_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.sections.add()
    journey1.sections[0].id = "section_1"
    journey1.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections.add()
    journey1.sections[1].id = "section_2"
    journey1.sections[1].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections.add()
    journey1.sections[2].id = "section_3"
    journey1.sections[2].type = response_pb2.PUBLIC_TRANSPORT

    journey2 = response.journeys.add()
    journey2.sections.add()
    journey2.sections[0].id = "section_4"
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections.add()
    journey2.sections[1].id = "section_5"
    journey2.sections[1].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections.add()
    journey2.sections[2].id = "section_6"
    journey2.sections[2].type = response_pb2.PUBLIC_TRANSPORT

    ticket1 = response.tickets.add()
    ticket1.id = "ticket_1"
    ticket1.cost.value = 100.0
    ticket1.cost.currency = "centime"
    ticket1.section_id.extend(["section_1", "section_2", "section_5"])

    ticket2 = response.tickets.add()
    ticket2.id = "ticket_2"
    ticket2.cost.value = 200.0
    ticket2.cost.currency = "centime"
    ticket2.section_id.extend(["section_6"])

    jf.remove_excess_tickets_or_ticket_links(response)

    # Nothing has been removed
    assert len(response.tickets) == 2
    assert response.tickets[0].section_id == ["section_1", "section_2", "section_5"]
    assert response.tickets[1].section_id == ["section_6"]

    # Now journey2 is to be deleted
    journey2.tags.extend(["to_delete"])

    jf.remove_excess_tickets_or_ticket_links(response)

    assert len(response.tickets) == 1
    assert response.tickets[0].id == "ticket_1"
    assert response.tickets[0].section_id == ["section_1", "section_2"]

