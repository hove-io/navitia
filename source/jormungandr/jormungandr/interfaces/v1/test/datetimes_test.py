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
from flask.ext.restful import marshal_with
from jormungandr.interfaces.v1 import Journeys
from jormungandr.interfaces.v1.fields import DateTime
from jormungandr.scripts.tests.utils import to_time_stamp
from navitiacommon import response_pb2


class mock_obj:
    def __init__(self, tz=None, dt=None, nested=None):
        self.timezone = tz
        if dt:
            self.date_time = to_time_stamp(dt)
        self.nested = nested


def datetime_formater_test():
    mock = mock_obj("Europe/Paris", "20140705T100000")

    formater = DateTime(timezone='timezone')

    print formater.output('date_time', mock)
    assert formater.output('date_time', mock) == "20140705T120000"


def datetime_nested_timezone_formater_test():
    """test with several nested attribute"""
    mock = mock_obj(dt="20131107T100000", nested=mock_obj(nested=mock_obj(tz="America/Los_Angeles")))

    formater = DateTime(timezone='nested.nested.timezone')

    print formater.output('date_time', mock)
    assert formater.output('date_time', mock) == "20131107T020000"


def journey_datetime_formater_test():
    """
    journey starts at 10UTC and finish at 13

    3 sections (each last 1 hour):
    A->B->C->D

    A is in paris
    B is in london
    C is in Abidjan (nice tz since they have neither DST nor utc offset, it is equivalent to UTC)
    D is in Los Angeles
    """
    pb_journey = response_pb2.Journey()
    pb_journey.departure_date_time = to_time_stamp("20140705T100000")
    pb_journey.arrival_date_time = to_time_stamp("20140705T130000")
    pb_journey.duration = 60 * 60
    pb_journey.origin.stop_area.timezone = "Europe/Paris"
    pb_journey.destination.stop_area.timezone = "America/Los_Angeles"
    pb_journey.sections.add()
    pb_journey.sections.add()
    pb_journey.sections.add()
    pb_journey.sections.add()

    section = pb_journey.sections[0]
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.origin.stop_area.timezone = "Europe/Paris"
    section.destination.stop_area.timezone = "Europe/London"
    section.begin_date_time = to_time_stamp("20140705T100000")
    section.end_date_time = to_time_stamp("20140705T110000")

    section = pb_journey.sections[1]
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.origin.stop_area.timezone = "Europe/London"
    section.destination.stop_area.timezone = "Africa/Abidjan"
    section.begin_date_time = to_time_stamp("20140705T110000")
    section.end_date_time = to_time_stamp("20140705T120000")

    section = pb_journey.sections[2]
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.origin.stop_area.timezone = "Africa/Abidjan"
    section.destination.stop_area.timezone = "America/Los_Angeles"
    section.begin_date_time = to_time_stamp("20140705T120000")
    section.end_date_time = to_time_stamp("20140705T130000")

    @marshal_with(Journeys.journey)
    def _mock():
        return pb_journey
    api_output = _mock()

    assert api_output["departure_date_time"] == "20140705T120000"  # 10h utc -> 12h paris in summer
    assert api_output["arrival_date_time"] == "20140705T060000"  # 13h utc -> 6h LA

    assert len(api_output["sections"]) == 4

    output_section = api_output["sections"][0]
    assert output_section["departure_date_time"] == "20140705T120000"
    assert output_section["from"]["stop_area"]["timezone"] == "Europe/Paris"
    assert output_section["arrival_date_time"] == "20140705T120000"  # 11h utc -> 12h london
    assert output_section["to"]["stop_area"]["timezone"] == "Europe/London"

    output_section = api_output["sections"][1]
    assert output_section["departure_date_time"] == "20140705T120000"
    assert output_section["from"]["stop_area"]["timezone"] == "Europe/London"
    assert output_section["arrival_date_time"] == "20140705T120000"  # 12h utc -> 12h abidjan
    assert output_section["to"]["stop_area"]["timezone"] == "Africa/Abidjan"

    output_section = api_output["sections"][2]
    assert output_section["departure_date_time"] == "20140705T120000"
    assert output_section["from"]["stop_area"]["timezone"] == "Africa/Abidjan"
    assert output_section["arrival_date_time"] == "20140705T060000"
    assert output_section["to"]["stop_area"]["timezone"] == "America/Los_Angeles"