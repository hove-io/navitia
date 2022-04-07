# coding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
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

from __future__ import absolute_import, print_function, division
from tyr.helper import is_activate_autocomplete_version
from tyr import app


def test_is_activate_autocomplete_version_with_mimir2(enable_mimir2):
    with app.app_context():
        assert is_activate_autocomplete_version(2)
        assert not is_activate_autocomplete_version(7)


def test_is_activate_autocomplete_version_with_mimir2_mimir7(enable_mimir2_and_mimir):
    with app.app_context():
        assert is_activate_autocomplete_version(2)
        assert is_activate_autocomplete_version(7)


def test_is_activate_autocomplete_version_with_mimir7(enable_mimir7):
    with app.app_context():
        assert not is_activate_autocomplete_version(2)
        assert is_activate_autocomplete_version(7)


def test_is_activate_autocomplete_version_without_mimir(disable_mimir):
    with app.app_context():
        assert not is_activate_autocomplete_version(2)
        assert not is_activate_autocomplete_version(7)
