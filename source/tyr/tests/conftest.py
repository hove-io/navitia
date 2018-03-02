# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import, print_function, unicode_literals, division
from tyr import app, db
import pytest

from tests.docker_wrapper import PostgresDocker


@pytest.yield_fixture(scope="session", autouse=True)
def docker():
    """
    a docker providing a database is started once for all tests
    """
    with PostgresDocker() as database:
        yield database



@pytest.fixture(scope="session", autouse=True)
def init_flask_db(docker):
    """
    when the docker is started, we init flask once for the new database
    """
    db_url = 'postgresql://{user}:{pwd}@{host}/{dbname}'.format(
                user=docker.USER,
                pwd=docker.PWD,
                host=docker.ip_addr,
                dbname=docker.DBNAME)

    # re-init the db by overriding the db_url
    app.config['SQLALCHEMY_DATABASE_URI'] = db_url
    db.init_app(app)
