# encoding: utf-8
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
from jormungandr.interfaces.v1.ResourceUri import complete_links


class MockResource(object):
    def __init__(self):
        self.region = 'test'


def mock_stop_schedules():
    return {
        "stop_schedules": [
            {
                "date_times": [
                    {
                        "date_time": "20160102T090052",
                        "links": [
                            {
                                "category": "comment",
                                "value": "Terminus (Quimper)",
                                "internal": "True",
                                "rel": "notes",
                                "type": "notes",
                                "id": "note:b5b328cb593ae7b1d73228345fe634fc",
                            },
                            {
                                "internal": "True",
                                "except_type": 1,
                                "rel": "exceptions",
                                "date": "20120619",
                                "type": "exceptions",
                                "id": "exception:120120619",
                            },
                        ],
                    },
                    {
                        "date_time": "20160102T091352",
                        "links": [
                            {
                                "category": "comment",
                                "value": "Terminus (Quimper)",
                                "internal": "True",
                                "rel": "notes",
                                "type": "notes",
                                "id": "note:b5b328cb593ae7b1d73228345fe634fc",
                            },
                            {
                                "internal": "True",
                                "except_type": 0,
                                "rel": "exceptions",
                                "date": "20120619",
                                "type": "exceptions",
                                "id": "exception:120120620",
                            },
                        ],
                    },
                ]
            }
        ]
    }


def test_complete_links():
    links = complete_links(MockResource())
    response = links(mock_stop_schedules)()
    assert "notes" in response
    assert "exceptions" in response
    assert len(response["exceptions"]) == 2
    assert len(response["notes"]) == 1
    for item in response["notes"][0].keys():
        assert item in ["category", "type", "id", "value", "comment_type"]

    for exception in response["exceptions"]:
        for item in exception.keys():
            assert item in ["date", "type", "id"]

    assert len(response["stop_schedules"]) == 1
    stop_schedules = response["stop_schedules"]
    assert len(stop_schedules[0]["date_times"]) == 2
    assert len(stop_schedules[0]["date_times"][0]["links"]) == 2
    assert len(stop_schedules[0]["date_times"][1]["links"]) == 2

    for dt in stop_schedules[0]["date_times"]:
        for link in dt["links"]:
            items = link.keys()
            for key in items:
                assert key in ["category", "internal", "rel", "type", "id"]
