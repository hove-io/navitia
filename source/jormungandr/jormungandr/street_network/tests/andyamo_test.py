# coding=utf-8
#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import
import pytest
from mock import MagicMock
import pybreaker
from navitiacommon import response_pb2
import navitiacommon.type_pb2 as type_pb2
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
import jormungandr.exceptions
from jormungandr.exceptions import UnableToParse
from jormungandr.street_network.andyamo import Andyamo
from jormungandr.utils import (
    str_to_time_stamp,
    PeriodExtremity,
)


class MockResource(object):
    def __init__(self, text=None, status=200):
        self.text = text
        self.status_code = status


def matrix_response_valid(response_id=0):
    # response_id=0 : len(sources) == len(targets) # Error
    # response_id=1 : len(sources) > len(targets)  # Error
    # response_id=2 : len(sources) < len(targets)  # Valid
    responses = {
        0: {
            "sources_to_targets": [
                {"from_index": 0, "to_index": 0, "time": 0, "distance": 0.0},
                {"distance": 0.035, "time": 28, "to_index": 1, "from_index": 0},
                {"from_index": 0, "to_index": 2, "time": 107, "distance": 0.134},
            ],
            "locations": {
                "targets": [
                    {"lat": 45.758373, "lon": 4.833177},
                    {"lat": 45.75817, "lon": 4.833374},
                    {"lat": 45.758923, "lon": 4.833948},
                ],
                "sources": [
                    {"lat": 45.75843, "lon": 4.83307},
                    {"lat": 45.75843, "lon": 4.83307},
                    {"lat": 45.75843, "lon": 4.83307},
                ],
            },
        },
        1: {
            "sources_to_targets": [
                {"from_index": 0, "to_index": 0, "time": 0, "distance": 0.0},
                {"distance": 0.035, "time": 28, "to_index": 1, "from_index": 0},
                {"from_index": 0, "to_index": 2, "time": 107, "distance": 0.134},
            ],
            "locations": {
                "targets": [
                    {"lat": 45.758373, "lon": 4.833177},
                    {"lat": 45.75817, "lon": 4.833374},
                ],
                "sources": [
                    {"lat": 45.75843, "lon": 4.83307},
                    {"lat": 45.75843, "lon": 4.83307},
                    {"lat": 45.75843, "lon": 4.83307},
                ],
            },
        },
        2: {
            "sources_to_targets": [
                {"from_index": 0, "to_index": 0, "time": 0, "distance": 0.0},
                {"distance": 0.035, "time": 28, "to_index": 1, "from_index": 0},
                {"from_index": 0, "to_index": 2, "time": 107, "distance": 0.134},
            ],
            "locations": {
                "targets": [
                    {"lat": 45.758373, "lon": 4.833177},
                    {"lat": 45.75817, "lon": 4.833374},
                    {"lat": 45.758923, "lon": 4.833948},
                ],
                "sources": [
                    {"lat": 45.75843, "lon": 4.83307},
                ],
            },
        },
    }
    return responses[response_id]


def direct_path_response_valid():
    return {
        "trip": {
            "summary": {
                "has_time_restrictions": False,
                "has_toll": False,
                "min_lon": 4.83523,
                "time": 2245,
                "has_highway": False,
                "has_ferry": False,
                "min_lat": 45.744082,
                "max_lat": 45.762182,
                "length": 2.80708,
                "max_lon": 4.852355,
                "cost": 2245,
            },
            "units": "kilometers",
            "legs": [
                {
                    "maneuvers": [
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.012,
                            "type": 0,
                        },
                        {
                            "type": 0,
                            "length": 0.033,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Rue du Palais Grillet"],
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.01,
                            "type": 15,
                        },
                        {
                            "street_names": ["Rue Thomassin"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.012,
                            "type": 15,
                        },
                        {
                            "type": 10,
                            "length": 0.004,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Rue de la République"],
                        },
                        {
                            "type": 15,
                            "length": 0.01,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Rue Thomassin"],
                        },
                        {
                            "type": 10,
                            "length": 0.062,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Rue de la République"],
                        },
                        {
                            "type": 15,
                            "length": 0.038,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Place de la République"],
                        },
                        {
                            "street_names": ["Rue du Président Carnot"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.009,
                            "type": 10,
                        },
                        {
                            "street_names": ["Place de la République"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.008,
                            "type": 15,
                        },
                        {
                            "street_names": ["Place de la République"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.042,
                            "type": 10,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.009,
                            "type": 10,
                        },
                        {
                            "street_names": ["Place de la République"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.039,
                            "type": 10,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.015,
                            "type": 10,
                        },
                        {
                            "street_names": ["Rue Marcel-Gabriel Rivière"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.1,
                            "type": 10,
                        },
                        {
                            "street_names": ["Place de l'Hôpital"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.02,
                            "type": 10,
                        },
                        {
                            "type": 10,
                            "length": 0.165,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Rue Bellecordière"],
                        },
                        {
                            "type": 10,
                            "length": 0.015,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Passage Pieton"],
                        },
                        {
                            "type": 15,
                            "length": 0.075,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Rue de la Barre"],
                        },
                        {
                            "type": 15,
                            "length": 0.025,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Passage Pieton"],
                        },
                        {
                            "type": 15,
                            "length": 0.011,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Route Inconnue"],
                        },
                        {
                            "type": 15,
                            "length": 0.017,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Passage Pieton"],
                        },
                        {
                            "type": 15,
                            "length": 0.264,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Pont de la Guillotière"],
                        },
                        {
                            "type": 15,
                            "length": 0.025,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Passage Pieton"],
                        },
                        {
                            "type": 15,
                            "length": 0.029,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Cours Gambetta"],
                        },
                        {
                            "type": 15,
                            "length": 0.01,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Passage Pieton"],
                        },
                        {
                            "type": 15,
                            "length": 0.146,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Cours Gambetta"],
                        },
                        {
                            "type": 15,
                            "length": 0.018,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Passage Pieton"],
                        },
                        {
                            "type": 15,
                            "length": 0.011,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Place Gabriel Péri"],
                        },
                        {
                            "type": 15,
                            "length": 0.103,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Grande Rue de la Guillotière"],
                        },
                        {
                            "type": 15,
                            "length": 0.009,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Passage Pieton"],
                        },
                        {
                            "type": 15,
                            "length": 0.053,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Grande Rue de la Guillotière"],
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.01,
                            "type": 15,
                        },
                        {
                            "street_names": ["Grande Rue de la Guillotière"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.113,
                            "type": 15,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.011,
                            "type": 15,
                        },
                        {
                            "street_names": ["Grande Rue de la Guillotière"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.054,
                            "type": 15,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.01,
                            "type": 15,
                        },
                        {
                            "street_names": ["Grande Rue de la Guillotière"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.027,
                            "type": 15,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.019,
                            "type": 15,
                        },
                        {
                            "street_names": ["Grande Rue de la Guillotière"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.025,
                            "type": 15,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.018,
                            "type": 15,
                        },
                        {
                            "street_names": ["Grande Rue de la Guillotière"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.083,
                            "type": 15,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.009,
                            "type": 15,
                        },
                        {
                            "street_names": ["Grande Rue de la Guillotière"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.067,
                            "type": 15,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.014,
                            "type": 15,
                        },
                        {
                            "street_names": ["Rue de la Madeleine"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.235,
                            "type": 10,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.009,
                            "type": 10,
                        },
                        {
                            "street_names": ["Rue de la Madeleine"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.088,
                            "type": 10,
                        },
                        {
                            "street_names": ["Rue du Repos"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.004,
                            "type": 10,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.013,
                            "type": 10,
                        },
                        {
                            "street_names": ["Rue du Repos"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.058,
                            "type": 10,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.019,
                            "type": 10,
                        },
                        {
                            "street_names": ["Rue du Repos"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.099,
                            "type": 10,
                        },
                        {
                            "street_names": ["Rue du Repos"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.016,
                            "type": 10,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.009,
                            "type": 10,
                        },
                        {
                            "street_names": ["Rue du Repos"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.015,
                            "type": 10,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.038,
                            "type": 15,
                        },
                        {
                            "street_names": ["Rue du Repos"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.054,
                            "type": 15,
                        },
                        {
                            "street_names": ["Passage Pieton"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.009,
                            "type": 15,
                        },
                        {
                            "street_names": ["Rue du Repos"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.033,
                            "type": 15,
                        },
                        {
                            "street_names": ["Parc Sergent Blandan"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.026,
                            "type": 15,
                        },
                        {
                            "street_names": ["Rue de l'Épargne"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.088,
                            "type": 10,
                        },
                        {
                            "street_names": ["Route Inconnue"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.026,
                            "type": 10,
                        },
                        {
                            "street_names": ["Rue de l'Épargne"],
                            "travel_type": "foot",
                            "travel_mode": "pedestrian",
                            "cost": 0,
                            "time": 15,
                            "instruction": "instruction placeholder",
                            "length": 0.099,
                            "type": 10,
                        },
                        {
                            "type": 15,
                            "length": 0.01,
                            "instruction": "instruction placeholder",
                            "time": 15,
                            "cost": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                            "street_names": ["Parc Sergent Blandan"],
                        },
                    ],
                    "summary": {
                        "has_time_restrictions": False,
                        "has_toll": False,
                        "min_lon": 4.83523,
                        "time": 2245,
                        "has_highway": False,
                        "has_ferry": False,
                        "min_lat": 45.744082,
                        "max_lat": 45.762182,
                        "length": 2.80708,
                        "max_lon": 4.852355,
                        "cost": 2245,
                    },
                    "shape": "kgbhvA{xbfH|EO|Qg@IiGGyHvA@DwGfb@pAV{]|CnBJaFnTtMrDFhUl@rGLpIHz`@aLdJuCfJvA~vA|i@bGtB~Oev@bBkHxBiHjA_HvB_Lby@ibEbEqQlCcHhBgKjA}FtPct@pJw^bFyTv@oE~@oFbBeGzE{FbYgK|QgQzCsB|WiW`D}BdBqBbc@}e@|G}OpBoFdQya@zB{D|HkOfF}JfIwM|FgHpJiVxReYbCeDxX_g@jEeFlHlD~jAxEhRfApSj@`FP`DSbKh@|JLhVu@|A]t@cAnFFhSkd@dF{J~d@gy@fGtElDWdDkIyDsEfCqIBsGjXeXdCwC|CsEtHqI_CqS~k@s\\hMvDxp@ac@oBiE",
                }
            ],
            "status": 0,
            "language": "fr-FR",
            "status_message": "Status Placeholder",
        },
        "id": "andyamo_directions",
    }


fake_service_url = "andyamo.com"
fake_asgard_url = "asgard.andyamo.com"
fake_asgard_socket = "tcp://socket.andyamo.com:666"
service_backup = {
    "args": {"service_url": fake_asgard_url, "asgard_socket": fake_asgard_socket},
    "class": "jormungandr.street_network.asgard.Asgard",
}


def test_create_andyamo_without_service_backup():
    instance = MagicMock()
    with pytest.raises(ValueError) as excinfo:
        Andyamo(instance=instance, service_url=fake_service_url, service_backup=None, zone='')
    assert str(excinfo.value) == 'service_backup None is not defined hence can not forward to asgard'


def test_create_andyamo_without_service_url():
    instance = MagicMock()
    with pytest.raises(ValueError) as excinfo:
        Andyamo(instance=instance, service_url=None, service_backup=service_backup, zone='')
    assert str(excinfo.value) == 'service_url None is not a valid andyamo url'


def test_create_andyamo_with_default_values():
    instance = MagicMock()

    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    assert andyamo.sn_system_id == "andyamo"
    assert andyamo.timeout == 10
    assert andyamo.service_url == fake_service_url
    assert len(andyamo.modes) == 1
    assert andyamo.modes[0] == "walking"
    assert andyamo.breaker.reset_timeout == 60
    assert andyamo.breaker.fail_max == 4
    assert andyamo._feed_publisher.name == "andyamo"
    assert andyamo._feed_publisher.id == "andyamo"
    assert andyamo._feed_publisher.license == "Private"
    assert andyamo._feed_publisher.url == "https://www.andyamo.fr"


def test_create_andyamo_with_config_test():
    instance = MagicMock()
    kwargs = {"circuit_breaker_max_fail": 2, "circuit_breaker_reset_timeout": 30}
    andyamo = Andyamo(
        instance=instance,
        id="id_handmap",
        service_url=fake_service_url,
        service_backup=service_backup,
        zone='',
        timeout=5,
        **kwargs,
    )
    assert andyamo.sn_system_id == "id_handmap"
    assert andyamo.timeout == 5
    assert andyamo.service_url == fake_service_url
    assert len(andyamo.modes) == 1
    assert andyamo.modes[0] == "walking"
    assert andyamo.breaker.reset_timeout == 30
    assert andyamo.breaker.fail_max == 2
    assert andyamo._feed_publisher.name == "andyamo"
    assert andyamo._feed_publisher.id == "andyamo"
    assert andyamo._feed_publisher.license == "Private"
    assert andyamo._feed_publisher.url == "https://www.andyamo.fr"


def test_create_andyamo_status_test():
    instance = MagicMock()
    kwargs = {"circuit_breaker_max_fail": 2, "circuit_breaker_reset_timeout": 30}
    andyamo = Andyamo(
        instance=instance,
        id="id_handmap",
        service_url=fake_service_url,
        modes=["walking"],
        service_backup=service_backup,
        zone='',
        timeout=5,
        **kwargs,
    )
    status = andyamo.status()
    assert status["id"] == "id_handmap"
    assert len(status["modes"]) == 1
    assert status["modes"][0] == "walking"
    assert status["timeout"] == 5


def call_andyamo_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    andyamo.breaker = MagicMock()
    andyamo.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())

    try:
        andyamo._call_andyamo(andyamo.service_url, data={})
        assert False, "AndyamoTechnicalError expected but not raised"
    except jormungandr.exceptions.AndyamoTechnicalError as e:
        assert str(e) == 'Andyamo routing service unavailable, Circuit breaker is open'
    except Exception as e:
        assert False, f"Unexpected exception type: {type(e).__name__}"


def call_andyamo_func_with_unknown_exception_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    andyamo.breaker = MagicMock()
    andyamo.breaker.call = MagicMock(side_effect=ValueError())
    with pytest.raises(jormungandr.exceptions.AndyamoTechnicalError) as andyamo_exception:
        andyamo._call_andyamo(andyamo.service_url, data={})
    assert str(andyamo_exception.value) == '500 Internal Server Error: None'


def check_response_and_get_json_andyamo_func_code_invalid_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp = MockResource(status=401)
    with pytest.raises(jormungandr.exceptions.AndyamoTechnicalError) as andyamo_exception:
        andyamo.check_response_and_get_json(resp)
    assert str(andyamo_exception.value) == '500 Internal Server Error: None'


def check_response_and_get_json_andyamo_func_json_invalid_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp = MockResource(text="toto")
    with pytest.raises(jormungandr.exceptions.UnableToParse) as andyamo_exception:
        andyamo.check_response_and_get_json(resp)
    assert str(andyamo_exception.value) == "400 Bad Request: None"


def format_coord_andyamo_func_test():
    pt_object = type_pb2.PtObject()
    pt_object.embedded_type = type_pb2.POI
    pt_object.poi.coord.lon = 42.42
    pt_object.poi.coord.lat = 41.41
    coords = Andyamo._format_coord(pt_object)
    assert coords["lon"] == pt_object.poi.coord.lon
    assert coords["lat"] == pt_object.poi.coord.lat


def get_response_andyamo_represents_start_true_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=-1.6761174, lat=48.1002462, uri='AndyamoStart')
    destination = make_pt_object(type_pb2.ADDRESS, lon=-1.6740057, lat=48.097592, uri='AndyamoEnd')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20220503T060000'), True)

    proto_resp = andyamo._get_response(
        json_response=resp_json,
        pt_object_origin=origin,
        pt_object_destination=destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20220503T060000')},
    )

    assert len(proto_resp.journeys) == 1
    assert proto_resp.journeys[0].durations.total == 2245
    assert proto_resp.journeys[0].durations.walking == 2245
    assert proto_resp.journeys[0].distances.walking == 2807
    assert proto_resp.journeys[0].requested_date_time == str_to_time_stamp('20220503T060000')

    assert len(proto_resp.journeys[0].sections) == 1
    assert proto_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
    assert proto_resp.journeys[0].sections[0].origin.uri == "AndyamoStart"
    assert proto_resp.journeys[0].sections[0].destination.uri == "AndyamoEnd"
    assert proto_resp.journeys[0].sections[0].street_network.length == 2807
    assert proto_resp.journeys[0].sections[0].street_network.duration == 2245
    assert proto_resp.journeys[0].sections[0].street_network.mode == response_pb2.Walking
    assert proto_resp.journeys[0].arrival_date_time == str_to_time_stamp('20220503T060000') + 2245


def get_response_andyamo_represents_start_false_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=-1.6761174, lat=48.1002462, uri='AndyamoStart')
    destination = make_pt_object(type_pb2.ADDRESS, lon=-1.6740057, lat=48.097592, uri='AndyamoEnd')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20220503T060000'), False)

    proto_resp = andyamo._get_response(
        json_response=resp_json,
        pt_object_origin=origin,
        pt_object_destination=destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20220503T055500')},
    )

    assert len(proto_resp.journeys) == 1
    assert proto_resp.journeys[0].durations.total == 2245  # Adjusted to match the response
    assert proto_resp.journeys[0].durations.walking == 2245  # Adjusted to match the response
    # Compare the distance allowing a small difference
    assert (
        abs(proto_resp.journeys[0].distances.walking - 2807.08) < 0.1
    )  # Allow a small difference due to rounding
    assert proto_resp.journeys[0].requested_date_time == str_to_time_stamp('20220503T055500')


def test_get_response_int_cast_success():  # Préparation des données de test
    json_response = direct_path_response_valid()

    # Création des mocks pour pt_object_origin et pt_object_destination
    pt_object_origin = type_pb2.PtObject()
    pt_object_destination = type_pb2.PtObject()

    # Autres paramètres nécessaires pour le test
    datetime = MagicMock()
    fallback_extremity = (datetime, True)

    # Création de l'instance Andyamo
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')

    # Appel de la méthode _get_response
    resp = andyamo._get_response(
        json_response=json_response,
        pt_object_origin=pt_object_origin,
        pt_object_destination=pt_object_destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20220503T123000')},
    )

    assert resp.journeys[0].sections[0].street_network.path_items[0].direction == 0
    assert resp.journeys[0].sections[0].street_network.path_items[2].direction == -90
    assert resp.journeys[0].sections[0].street_network.path_items[4].direction == 90
    assert resp.journeys[0].requested_date_time == str_to_time_stamp('20220503T123000')


def create_pt_object(lon, lat, pt_object_type=type_pb2.POI):
    pt_object = type_pb2.PtObject()
    pt_object.embedded_type = pt_object_type
    if pt_object_type == type_pb2.POI:
        pt_object.poi.coord.lon = lon
        pt_object.poi.coord.lat = lat
    elif pt_object_type == type_pb2.ADDRESS:
        pt_object.address.coord.lon = lon
        pt_object.address.coord.lat = lat
    return pt_object


def check_content_response_andyamo_func_valid_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(2)
    origins = [create_pt_object(4.83307, 45.75843)]
    destinations = [
        create_pt_object(4.833177, 45.758373),
        create_pt_object(4.833374, 45.75817),
        create_pt_object(4.833948, 45.758923),
    ]
    # Assuming check_content_response is a method of Andyamo that does not return a value but raises an exception if invalid
    try:
        andyamo.check_content_response(resp_json, origins, destinations)
        assert True  # Pass the test if no exception is raised
    except Exception as e:
        pytest.fail(f"Unexpected exception raised: {e}")


def call_andyamo_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    andyamo.breaker = MagicMock()
    andyamo.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())
    with pytest.raises(jormungandr.exceptions.AndyamoTechnicalError) as andyamo_exception:
        andyamo._call_andyamo(andyamo.service_url, data={})
    assert '500 Internal Server Error: None' in str(andyamo_exception.value)


def create_matrix_response_andyamo_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(1)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]
    sn_matrix = andyamo._create_matrix_response(resp_json, origins, destinations, 150)
    assert len(sn_matrix.rows) == 1
    assert len(sn_matrix.rows[0].routing_response) == 3


def check_content_response_andyamo_func_fail_0_test():
    # response_id=0 : len(sources) == len(targets) # Expected to fail
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(0)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340)]

    # Le test doit maintenant s'attendre à une exception
    with pytest.raises(UnableToParse):
        andyamo.check_content_response(resp_json, origins, destinations)


def check_content_response_andyamo_func_valid_1_test():
    # response_id=1 : len(sources) > len(targets)
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(1)
    origins = [
        create_pt_object(4.83307, 45.75843),
        create_pt_object(4.83307, 45.75843),
        create_pt_object(4.83307, 45.75843),
    ]
    destinations = [create_pt_object(4.833177, 45.758373), create_pt_object(4.833374, 45.75817)]
    # Assuming check_content_response is a method of Andyamo that does not return a value but raises an exception if invalid
    try:
        andyamo.check_content_response(resp_json, origins, destinations)
        assert True  # Pass the test if no exception is raised
    except Exception as e:
        pytest.fail(f"Unexpected exception raised: {e}")


def check_content_response_andyamo_func_valid_2_test():
    # response_id=2 : len(sources) < len(targets)
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(2)
    origins = [create_pt_object(-1.679860, 48.109340)]
    destinations = [
        create_pt_object(-1.680150, 48.108770),
        create_pt_object(-1.678750, 48.109390),
        create_pt_object(-1.678750, 48.109390),
    ]

    # Assuming check_content_response is a method of Andyamo that does not return a value but raises an exception if invalid
    try:
        andyamo.check_content_response(resp_json, origins, destinations)
        assert True  # Pass the test if no exception is raised
    except Exception as e:
        pytest.fail(f"Unexpected exception raised: {e}")
