# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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

import zipfile


def type_of_data(filename, only_one_file=True):
    """
    return the type of data contains in a file

    this type can be one in:
     - 'gtfs'
     - 'fusio'
     - 'fare'
     - 'osm'
     - 'geopal'
     - 'fusio'
     - 'poi'
     - 'synonym'
     - 'shape'

     if only_one_file is True, so consider only a zip for all pt data
     else we consider also them for multi files
    """
    def pt_types(files):
        #first we try fusio, because it can load fares too
        if "contributors.txt" in files:
            return 'fusio'
        if 'fares.csv' in files:
            return 'fare'
        if 'stops.txt' in files:
            return 'gtfs'
        return None

    if filename.endswith('.pbf'):
        return 'osm'
    if filename.endswith('.zip'):
        zipf = zipfile.ZipFile(filename)
        return pt_types(zipf.namelist())
    if filename.endswith('.geopal'):
        return 'geopal'
    if filename.endswith('.poi'):
        return 'poi'
    if filename.endswith("synonyms.txt"):
        return 'synonym'
    if filename.endswith(".poly") or filename.endswith(".wkt"):
        return 'shape'

    if not only_one_file:
        return pt_types([filename])

    return None
