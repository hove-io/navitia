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
from contextlib import closing
from testscommon.docker_wrapper import PostgresDocker
from os.path import join, normpath, abspath, isfile
import os
import docker
import pytest
import psycopg2
import subprocess




@pytest.yield_fixture(scope="session")
def postgres_docker():
    """
    a docker providing a database is started once for all tests
    """
    path = None
    if os.getenv("NAVITIA_CHAOS_DUMP_PATH"):
        path = os.getenv("NAVITIA_CHAOS_DUMP_PATH")
    else:
        path = abspath("chaos_loading.sql.gz")
    sql_mount = docker.types.Mount("/docker-entrypoint-initdb.d/chaos_loading.sql.gz", path, type="bind")
    with closing(PostgresDocker("chaos_loading", mounts=[sql_mount])) as chaos_docker:
        yield chaos_docker


def test_loading_disruption_from_chaos(postgres_docker):
    chaos_tests_dir = os.getenv('CHAOS_DB_TESTS_BUILD_DIR', '.')
    chaos_tests_exec = normpath(join(chaos_tests_dir, 'chaos_db_tests'))

    assert isfile(chaos_tests_exec), "Couldn't find test executable : {}".format(chaos_tests_exec)

    # Give the cpp test a connection string to the db
    os.environ['CHAOS_DB_CONNECTION_STR'] = postgres_docker.get_db_params().old_school_cnx_string()
    assert subprocess.check_call([chaos_tests_exec]) == 0
