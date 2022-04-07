# Copyright (c) 2002-2014, Hove and/or its affiliates. All rights reserved.
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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from . import check_utils


def test_query_from_str():
    r = check_utils.query_from_str("toto/tata?bob=toto&bobette=tata&bobinos=tutu")
    assert r == {'bobette': 'tata', 'bobinos': 'tutu', 'bob': u'toto'}
    r = check_utils.query_from_str("toto/tata?bob=toto&bob=tata&bob=titi&bob=tata&bobinos=tutu")
    assert r == {'bobinos': 'tutu', 'bob': ['toto', 'tata', 'titi', 'tata']}
    r = check_utils.query_from_str("?bob%5B%5D=toto titi&bob[]=tata%2Btutu")
    assert r == {'bob[]': ['toto titi', 'tata+tutu']}
