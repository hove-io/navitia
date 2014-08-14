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

from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


@dataset(["main_routing_test"])
class TestPassages(AbstractTestFixture):
    """
    Test the structure of passages
    """

    def test_departures(self):
        url = "next_departures.json?filter=stop_area.uri=stopB&from_datetime=20120614T000000"
        response = self.query_region(url, display=False, version="v0")
        #nothing to test, the API is broken and noboy cares

    def test_arrivals(self):
        url = "next_departures.json?filter=stop_area.uri=stopB&from_datetime=20120614T000000"
        response = self.query_region(url, display=False, version="v0")
        #nothing to test, the API is broken and noboy cares

def check_departure_board_structure(board):
    return "hour" in board and "minutes" in board and type(board["minutes"]) == list

@dataset(["main_routing_test"])
class TestDepartureBoards(AbstractTestFixture):
    """
    Test content and structure of a departure_boards
    """


    def test_structure(self):
        url = "departure_boards.json?filter=stop_area.uri=stopB&from_datetime=20120614T000000"
        response = self.query_region(url, display=False, version="v0")
        assert("departure_boards" in response)
        departure_boards = response['departure_boards']
        assert(len(departure_boards) == 1)
        departure_board = departure_boards[0]
        assert("stop_point" in departure_board)
        assert("route" in departure_board)
        assert("board_items" in departure_board)
        board_items = departure_board["board_items"]
        assert(len(board_items) == 1)
        assert(check_departure_board_structure(board_items[0]))


    def test_content(self):
        url = "departure_boards.json?filter=stop_area.uri=stopB&from_datetime=20120614T000000"
        response = self.query_region(url, display=False, version="v0")
        assert("departure_boards" in response)
        assert(len(response["departure_boards"]) == 1)
        assert(len(response["departure_boards"][0]["board_items"]) == 1)
        assert(response["departure_boards"][0]["board_items"][0]["hour"] == "8")
        assert(len(response["departure_boards"][0]["board_items"][0]["minutes"]) == 1)
        assert(response["departure_boards"][0]["board_items"][0]["minutes"][0] == "1")

def check_struct_row(row):
    return type(row) == dict and "stop_point" in row and "stop_times" in row and type(row["stop_times"]) == list and\
        reduce(lambda a,b: a and b, map(lambda a : is_valid_datetime(a), row["stop_times"] ))

@dataset(["main_routing_test"])
class TestDepartureBoards(AbstractTestFixture):
    """
    Test content and structure of a route_schedules
    """


    def test_structure(self):
        url = "route_schedules.json?filter=stop_area.uri=stopB&from_datetime=20120614T000000"
        response = self.query_region(url, display=False, version="v0")
        assert("route_schedules" in response)
        route_schedules = response["route_schedules"]
        assert(type(route_schedules) == list)
        assert(len(route_schedules) == 1)
        route_schedule = route_schedules[0]
        assert("pt_display_informations" in route_schedule)
        informations = route_schedule["pt_display_informations"]
        assert(type(informations) == dict)
        assert("direction" in informations)
        assert("code" in informations)
        assert("network" in informations)
        assert("color" in informations)
        assert("uris" in informations)
        assert(type(informations["uris"]) == dict)
        assert("line" in informations["uris"])
        assert("route" in informations["uris"])
        assert("network" in informations["uris"])
        assert("name" in informations)
        assert("table" in route_schedule)
        table = route_schedule["table"]
        assert(type(table) == dict)
        assert("headers" in table)
        assert("rows" in table)
        headers = table["headers"]
        rows = table["rows"]
        assert(reduce(lambda a,b : a and b, map(check_struct_row, rows)))


    def test_content(self):
        url = "route_schedules.json?filter=stop_area.uri=stopB&from_datetime=20120614T000000"
        response = self.query_region(url, display=False, version="v0")
        assert(response["route_schedules"][0]["table"]["rows"][0]["stop_times"][0] == "20120614T080100")
        assert(response["route_schedules"][0]["table"]["rows"][1]["stop_times"][0] == "20120614T080102")


@dataset(["basic_routing_test"])
class TestJourneys(AbstractTestFixture):
    """
    Test if the filter on long waiting duration is working
    """

    def test_content(self):
        """
        On this call the first call to kraken returns a journey
        with a too long waiting duration.
        The second call to kraken must return a valid journey
        """
        query = "journeys.json?origin={from_sa}&destination={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False, version="v0")
        assert("journeys" in response)
        assert(type(response["journeys"]) == list)
        assert(len(response["journeys"]) == 1)
        journey = response["journeys"][0]
        assert("arrival_date_time" in journey)
        assert(is_valid_datetime(journey["arrival_date_time"]))
        assert(journey["arrival_date_time"] == "20120614T160000")
        assert(is_valid_datetime(journey["departure_date_time"]))
        assert(journey["departure_date_time"] == "20120614T150000")
        assert(is_valid_datetime(journey["requested_date_time"]))
        assert(journey["requested_date_time"] == "20120614T080100")
        assert("sections" in journey)
        assert(type(journey["sections"]) == list)
        assert(len(journey["sections"]) == 4)
        sections = journey["sections"]
        section = sections[0]
        assert(section["begin_date_time"] == "20120614T150000")
        assert(section["end_date_time"] == "20120614T150500")
        section = sections[1]
        assert(section["begin_date_time"] == "20120614T150500")
        assert(section["end_date_time"] == "20120614T150700")
        section = sections[2]
        assert(section["begin_date_time"] == "20120614T150700")
        assert(section["end_date_time"] == "20120614T151000")
        section = sections[3]
        assert(section["begin_date_time"] == "20120614T151000")
        assert(section["end_date_time"] == "20120614T160000")