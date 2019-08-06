# coding: utf-8
# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
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

from __future__ import absolute_import, print_function, division
from contextlib import closing
from navitiacommon.launch_exec import launch_exec_traces
from testscommon.docker_wrapper import PostgresDocker
import logging
import docker
import pytest
import psycopg2
import sys
import os


@pytest.yield_fixture(scope="session")
def postgres_docker():
    """
    a docker providing a database is started once for all tests
    """
    sql_mount = docker.types.Mount(
        "/docker-entrypoint-initdb.d/chaos_loading.sql", os.path.abspath("chaos_loading.sql"), type="bind"
    )
    with closing(PostgresDocker("chaos_loading", mounts=[sql_mount])) as chaos_docker:
        yield chaos_docker


@pytest.fixture(scope="session")
def chaos_db(postgres_docker):
    """
    when the docker is started, we init a chaos db based on a snapshot
    """
    db_cnx_params = postgres_docker.get_db_params().old_school_cnx_string()
    with psycopg2.connect(db_cnx_params) as conn:
        with conn.cursor() as cursor:
            yield cursor


def test_loading_chaos(chaos_db):
    chaos_db.execute("SELECT * FROM line_section;")
    print(chaos_db.fetchone())
