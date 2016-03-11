# coding=utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import, print_function, unicode_literals

def transform_id(id):
    splitted_coord = id.split(";")
    splitted_address = id.split(":")
    if len(splitted_coord) == 2:
        try:
            # check if lon and lat are convertible to float
            float(splitted_coord[0])
            float(splitted_coord[1])
            return "coord:" + id.replace(";", ":")
        except ValueError:
            pass
    if len(splitted_address) >= 3 and splitted_address[0] == 'address':
        del splitted_address[1]
        return ':'.join(splitted_address)
    if len(splitted_address) >= 3 and splitted_address[0] == 'admin':
        del splitted_address[1]
        return ':'.join(splitted_address)
    return id
