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
import os
import glob

street_source_types = ['OSM']
address_source_types = ['BANO', 'OSM']
poi_source_types = ['FUSIO', 'OSM']
admin_source_types = ['OSM']

def type_of_data(filename):
    """
    return the type of data contains in a file + the path to load it

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

    for 'fusio', 'gtfs', 'fares' and 'poi', we return the directory since there are several file to load
    """
    def files_type(files):
        #first we try fusio, because it can load fares too
        if any(f for f in files if f.endswith("contributors.txt")):
            return 'fusio'
        if any(f for f in files if f.endswith("fares.csv")):
            return 'fare'
        if any(f for f in files if f.endswith("stops.txt")):
            return 'gtfs'
        if any(f for f in files if f.endswith("adresse.txt")):
            return 'geopal'
        if any(f for f in files if f.endswith("poi.txt")):
            return 'poi'
        return None

    if not isinstance(filename, list):
        if os.path.isdir(filename):
            files = glob.glob(filename + "/*")
        else:
            files = [filename]
    else:
        files = filename

    # we test if we recognize a ptfile in the list of files
    t = files_type(files)
    if t:  # the path to load the data is the directory since there are several files
        return t, os.path.dirname(files[0])

    for filename in files:
        if filename.endswith('.pbf'):
            return 'osm', filename
        if filename.endswith('.zip'):
            zipf = zipfile.ZipFile(filename)
            pt_type = files_type(zipf.namelist())
            if not pt_type:
                return None, None
            return pt_type, filename
        if filename.endswith('.geopal'):
            return 'geopal', filename
        if filename.endswith('.poi'):
            return 'poi', os.path.dirname(filename)
        if filename.endswith("synonyms.txt"):
            return 'synonym', filename
        if filename.endswith(".poly") or filename.endswith(".wkt"):
            return 'shape', filename

    return None, None


def family_of_data(type):
    """
    return the family type of a data type
    by example "geopal" and "osm" are in the "streetnework" family
    """
    mapping = {
        'osm': 'streetnetwork', 'geopal': 'streetnetwork',
        'synonym': 'synonym',
        'poi': 'poi',
        'fusio': 'pt', 'gtfs': 'pt',
        'fare': 'fare',
        'shape': 'shape'
    }
    if type in mapping:
        return mapping[type]
    else:
        return None
