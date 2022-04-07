# -*- coding: utf-8 -*-
# Copyright (c) 2001-2020, Hove and/or its affiliates. All rights reserved.
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

from jormungandr.interfaces.v1.serializer.base import SortedGenericSerializer
import serpy


def test_sorted_generic_serializer():
    class SortSerializer(SortedGenericSerializer, serpy.DictSerializer):
        v = serpy.IntField()

        def sort_key(self, obj):
            return obj["v"]

    objs = [{'v': '2'}, {'v': '4'}, {'v': '3'}, {'v': '1'}]
    data = SortSerializer(objs, many=True).data
    assert data[0]['v'] == 1
    assert data[1]['v'] == 2
    assert data[2]['v'] == 3
    assert data[3]['v'] == 4
