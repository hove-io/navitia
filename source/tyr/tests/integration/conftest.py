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
import os
from tyr import app, db
import pytest
import flask_migrate


@pytest.yield_fixture(scope="module", autouse=True)
def bdd(init_flask_db):
    """
    All tests under this module will have a database
     with an up to date scheme

    At the end of the module the database scheme will be downgraded and upgraded again
    in the next module to test the database migrations
    """
    with app.app_context():
        flask_migrate.Migrate(app, db)
        migration_dir = os.path.join(os.path.dirname(__file__), '..', '..', 'migrations')
        flask_migrate.upgrade(directory=migration_dir)

    yield

    with app.app_context():
        flask_migrate.downgrade(revision='base', directory=migration_dir)


@pytest.fixture(scope='function', autouse=True)
def clean_db():
    """
    before all tests the database is cleared
    """
    with app.app_context():
        tables = ['"{}"'.format(table) for table in ['user', 'instance', 'authorization', 'key',
                                                     'data_set', 'job', 'poi_type_json',
                                                     'autocomplete_parameter']]
        db.session.execute('TRUNCATE {} CASCADE;'.format(', '.join(tables)))
        db.session.commit()

