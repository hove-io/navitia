# coding=utf-8

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

import sys
import logging

from contextlib import closing
from clingon import clingon
from typing import List

"""
Eitri uses a set of binaries to extract data, load it into a database and extract it to a single file.

First, it uses every binaries "*2ed" to create data to load in the database.
Then it uses "ed2nav" binary to extract that data from the database and create a single file ".nav.lz4".
In the end, the file ".nav.lz4" is often used as input coverage data for Kraken.
"""

logging.basicConfig(level=logging.DEBUG)


@clingon.clize()
def eitri(
    data_dir,
    output_file='./data.nav.lz4',
    ed_component_path='',
    cities_exec_path='',
    import_cities='',
    add_pythonpath=[],
):
    # type: (str, str, str, str, str, List[str]) -> None
    """
    Generate a "data.nav.lz4" file.

    :param data_dir: the directory with all data. If several datasets ("*.osm", "*.gtfs", ...) are available, they need to be in separate directories
    :param output_file: the output file path
    :param ed_component_path: the folder path containing all "*2ed" and "ed2nav" binaries
    :param cities_exec_path: the path to the binary "cities"
    :param import_cities: the path to cities file (ex: france_boundaries.osm.pbf) to load
    """

    # there is some problems with environment variables and cmake, so all args can be given through cli
    for p in add_pythonpath:
        sys.path.append(p)

    from ed_handler import generate_nav
    from navitiacommon.docker_wrapper import PostgisDocker

    with closing(PostgisDocker()) as docker_ed, closing(PostgisDocker()) as docker_cities:
        generate_nav(
            data_dir,
            docker_ed,
            docker_cities,
            output_file,
            ed_component_path=ed_component_path,
            cities_exec_path=cities_exec_path,
            import_cities=import_cities,
        )
