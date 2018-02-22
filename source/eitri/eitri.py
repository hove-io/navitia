#!python
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
from __future__ import absolute_import, print_function, unicode_literals, division
import sys

from clingon import clingon
import logging
"""
Nav generator

from an input dir create a data.nav.lz4 with the data needed by kraken
"""

logging.basicConfig(level=logging.DEBUG)


@clingon.clize()
def eitri(data_dir, output_file='./data.nav.lz4', ed_component_path='', add_pythonpath=[]):
    """
    Generate a data.nav.lz4 file

    :param data_dir: directory with data. if several dataset (osm/gtfs/...) are available, they need to be in separate directory
    :param output_file: output data.nav.lz4 file path
    """

    # there is some problems with environment variables and cmake, so all args
    # can be given through cli
    for p in add_pythonpath:
        sys.path.append(p)

    from ed_handler import generate_nav
    from docker_wrapper import PostgresDocker

    with PostgresDocker() as docker:
        generate_nav(data_dir, docker.get_db_params(), output_file, ed_component_path=ed_component_path)
