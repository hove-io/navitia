# Copyright (c) 2001-2024, Hove and/or its affiliates. All rights reserved.
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
import pytest
import shapely
from collections import namedtuple
from datetime import date

from jormungandr.excluded_zones_manager import ExcludedZonesManager

shape_str = "POLYGON ((2.3759783199572553 48.8446081592198, 2.3739259200592358 48.84555123669577, 2.372796492896299 48.84471205845662, 2.372590038468587 48.84448827521729, 2.3768770039353626 48.842122505530256, 2.3793180239319156 48.84329741187722, 2.3759783199572553 48.8446081592198))"


def mock_get_all_excluded_zones():

    return [
        {
            "id": "Gare de lyon 1",
            "instance": "toto",
            "poi": "poi:gare_de_lyon",
            "activation_periods": [{"from": "20240810", "to": "20240812"}],
            "modes": ["walking", "bike"],
            "shape": shape_str,
        },
        {
            "id": "Gare de lyon 2",
            "instance": "toto",
            "poi": "poi:gare_de_lyon",
            "activation_periods": [
                {"from": "20240801", "to": "20240802"},
                {"from": "20240810", "to": "20240812"},
            ],
            "modes": ["bike"],
            "shape": shape_str,
        },
    ]


@pytest.fixture(scope="function", autouse=True)
def mock_http_karos(monkeypatch):
    monkeypatch.setattr(
        'jormungandr.excluded_zones_manager.ExcludedZonesManager._get_all_excluded_zones',
        mock_get_all_excluded_zones,
    )


def get_excluded_zones_test():
    zones = ExcludedZonesManager.get_excluded_zones()
    assert len(zones) == 2

    zones = ExcludedZonesManager.get_excluded_zones(mode='walking')
    assert len(zones) == 1
    assert zones[0]["id"] == "Gare de lyon 1"

    request_date = date.fromisoformat('2024-08-10')
    zones = ExcludedZonesManager.get_excluded_zones(mode='bike', date=request_date)
    assert len(zones) == 2

    request_date = date.fromisoformat('2024-08-01')
    zones = ExcludedZonesManager.get_excluded_zones(mode='bike', date=request_date)
    assert len(zones) == 1
    assert zones[0]["id"] == "Gare de lyon 2"

    request_date = date.fromisoformat('2024-08-01')
    zones = ExcludedZonesManager.get_excluded_zones(mode='walking', date=request_date)
    assert len(zones) == 0


def excluded_manager_is_excluded_test():
    import datetime
    import pytz

    Coord = namedtuple('Coord', ['lon', 'lat'])

    ExcludedZonesManager.excluded_shapes["poi:gare_de_lyon"] = shapely.wkt.loads(shape_str)

    def ts_to_date(timestamp):
        return datetime.datetime.fromtimestamp(timestamp, tz=pytz.timezone("UTC")).date()

    res = ExcludedZonesManager.is_excluded(Coord(1, 1), mode='walking', date=ts_to_date(1723280205))
    assert not res

    res = ExcludedZonesManager.is_excluded(
        Coord(2.376365, 48.843402), mode='walking', date=ts_to_date(1723280205)
    )
    assert res

    res = ExcludedZonesManager.is_excluded(
        Coord(2.376365, 48.843402), mode='walking', date=ts_to_date(1722502605)
    )
    assert not res
