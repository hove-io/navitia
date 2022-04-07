#!/usr/bin/env python
# coding=utf-8

# Copyright (c) 2001-2015, Hove and/or its affiliates. All rights reserved.
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

import sys
import os

sys.path.append(os.path.dirname(__file__) + '/../navitiacommon')

import argparse
import logging
from contextlib import closing
from ed_handler import generate_nav
from navitiacommon import log
from testscommon.docker_wrapper import PostgisDocker

from typing import Any

"""
Eitri uses a set of binaries to extract data, load it into a database and extract it to a single file.

First, it uses every binaries "*2ed" to create data to load in the database.
Then it uses "ed2nav" binary to extract that data from the database and create a single file ".nav.lz4".
In the end, the file ".nav.lz4" is often used as input coverage data for Kraken.
"""


logger = logging.getLogger("eitri")  # type: logging.Logger


def is_directory(dir_path):
    if not os.path.isdir(dir_path):
        raise argparse.ArgumentTypeError("'" + dir_path + "' is not a valid directory path")
    return dir_path


def is_valid_file(file_path):
    dirname = os.path.dirname(file_path)
    if not os.access(dirname, os.W_OK):
        raise argparse.ArgumentTypeError("'" + file_path + "' is not a valid path")
    if os.access(file_path, os.F_OK):
        raise argparse.ArgumentTypeError("'" + file_path + "' already exist")
    if file_path[-1] == '/':
        raise argparse.ArgumentTypeError("'" + file_path + "' is not a valid file name")
    return file_path


def is_enabled(value):
    if isinstance(value, bool):
        return value
    if value.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif value.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError("'" + value + "' is not a boolean value")


def get_params():
    # type: () -> Any
    """
    Get and check all the parameters.

    :return: the path to the directory with all data. If several datasets ("*.osm", "*.gtfs", ...) are available, they need to be in separate directories
    :return: the output file path
    :return: the path of the directory containing all "*2ed" and "ed2nav" binaries
    :return: the path to the binary "cities"
    :return: the path to cities file (ex: france_boundaries.osm.pbf) to load
    """
    parser = argparse.ArgumentParser(
        prog='eitri',
        description='Eitri uses a set of binaries to extract data, load it into a database and extract it to a single file. '
        'This software is not distributed and only used by Navitia.',
        epilog='For more informations, see www.navitia.io',
    )
    parser.add_argument(
        '-d',
        '--data-dir',
        required=True,
        type=is_directory,
        help='the directory path with all data. If several datasets ("*.osm", "*.gtfs", ...) are available, they need to be in separate directories',
    )
    parser.add_argument(
        '-e',
        '--ed-dir',
        required=True,
        type=is_directory,
        help='the directory path containing all "*2ed" and "ed2nav" binaries',
    )
    parser.add_argument(
        '-o',
        '--output-file',
        type=is_valid_file,
        default='./data.nav.lz4',
        help="the output file path (default: './data.nav.lz4')",
    )
    parser.add_argument(
        '-i', '--cities-file', type=str, help='the path to the "cities" file to load  (usually a *.osm.pbf)'
    )
    parser.add_argument(
        '-f', '--cities-dir', type=is_directory, help='the path to the directory containing the "cities" binary'
    )
    parser.add_argument(
        '-c', '--color', type=is_enabled, default=True, help='enable or disable colored logs (default: True)'
    )
    return parser.parse_args()


def eitri():
    # type: () -> None
    """
    Generate a "data.nav.lz4" file.
    """
    args = get_params()
    if args.color:
        log.enable_color(True)
    else:
        log.enable_color(False)

    logger.info("starting Eitri")
    with closing(PostgisDocker()) as docker_ed, closing(PostgisDocker()) as docker_cities:
        generate_nav(
            args.data_dir,
            docker_ed,
            docker_cities,
            args.output_file,
            args.ed_dir,
            args.cities_file,
            args.cities_dir,
        )
    logger.info("file '" + args.output_file + "' is created !")


if __name__ == '__main__':
    eitri()
