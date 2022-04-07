# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
from jormungandr.autocomplete.geocodejson import format_zip_code
from jormungandr.interfaces.common import handle_poi_infos


def test_format_zip_code():
    zip_codes = []
    assert format_zip_code(zip_codes) == None

    zip_codes = [""]
    assert format_zip_code(zip_codes) == None

    zip_codes = ["", ""]
    assert format_zip_code(zip_codes) == None

    zip_codes = ["75000"]
    assert format_zip_code(zip_codes) == "75000"

    zip_codes = ["75012", "75013"]
    assert format_zip_code(zip_codes) == "75012-75013"

    zip_codes = ["75013", "75014", "75012"]
    assert format_zip_code(zip_codes) == "75012-75014"


def test_poi_infos():
    """
    Test the different possible values for the enum 'add_poi_infos[]'
    For retrocompatibility, test 'bss_stands' parameter that is now deprecated
    """
    # POI infos are not added if '' or 'none' is passed
    assert not handle_poi_infos([''], False)
    assert not handle_poi_infos(['none'], False)
    # Otherwise, it should be added.
    assert handle_poi_infos(['bss_stands', ''], False)
    assert handle_poi_infos(['car_park', 'none'], False)
    # For retrocompatibility, the parameter 'bss_stands' is still taken into account
    assert handle_poi_infos([''], True)
