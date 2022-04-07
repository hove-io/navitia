# coding: utf-8
# Copyright (c) 2001-2015, Hove and/or its affiliates. All rights reserved.
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
from tyr import app, db
import os
import pytest
import tempfile
import shutil

# TODO : need to clean that after full migration to python3
try:
    import ConfigParser
except:
    import configparser as ConfigParser  # type: ignore

from testscommon.docker_wrapper import PostgresDocker


@pytest.yield_fixture(scope="session", autouse=True)
def docker():
    """
    a docker providing a database is started once for all tests
    """
    with closing(PostgresDocker()) as database:
        yield database


@pytest.fixture(scope="session", autouse=True)
def init_flask_db(docker):
    """
    when the docker is started, we init flask once for the new database
    """
    db_url = 'postgresql://{user}:{pwd}@{host}/{dbname}'.format(
        user=docker.dbuser, pwd=docker.dbpassword, host=docker.ip_addr, dbname=docker.dbname
    )

    # re-init the db by overriding the db_url
    app.config['SQLALCHEMY_DATABASE_URI'] = db_url
    db.init_app(app)


def create_instance_config_file(instance_dir, backup_dir, name='default'):
    """
    Create a config file for an instance with path to the temp dir for the backup
    :param instance_dir: temp dir to store data for an instance
    :param backup_dir: dir to store datasets backup
    :param name: instance name
    """
    config = ConfigParser.ConfigParser()
    config.add_section('instance')
    config.set('instance', 'name', name)
    config.set('instance', 'source-directory', '/tmp')
    config.set('instance', 'backup-directory', backup_dir)
    config.set('instance', 'tmp-file', '/tmp/ed/tmpdata.nav.lz4')
    config.set('instance', 'target-file', '/tmp/ed/tmpdata.nav.lz4')

    config.add_section('database')
    config.set('database', 'host', '127.0.0.1')
    config.set('database', 'dbname', 'navitia')
    config.set('database', 'username', 'navitia')
    config.set('database', 'password', 'navitia')

    with open(os.path.join(instance_dir, '{}.ini'.format(name)), 'w') as configfile:
        config.write(configfile)


@pytest.fixture(scope="function", autouse=False)
def init_instances_dir():
    """
    Create a temp dir of an instance with its config file
    """
    instance_dir = tempfile.mkdtemp(prefix='instance_')
    fr_backup_dir = tempfile.mkdtemp(prefix='backup_fr_', dir=instance_dir)
    create_instance_config_file(instance_dir=instance_dir, backup_dir=fr_backup_dir, name='fr')
    app.config['INSTANCES_DIR'] = instance_dir

    yield

    shutil.rmtree(instance_dir)


@pytest.fixture(scope="function", autouse=False)
def init_cities_dir():
    """
    Create a temp dir of an instance with its config file
    """
    cities_dir = tempfile.mkdtemp(prefix='cities_')
    app.config['CITIES_OSM_FILE_PATH'] = cities_dir

    yield

    shutil.rmtree(cities_dir)
