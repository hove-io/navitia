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

import docker
import logging
import psycopg2
from typing import List, Optional, Dict
from retrying import retry
import os

"""
This module contains classes about Docker management.
"""

logger = logging.getLogger(__name__)  # type: logging.Logger


class DbParams(object):
    """
    Class to store and manipulate database parameters.
    """

    def __init__(self, host, dbname, user, password):
        # type: (str, str, str, str) -> None
        """
        Constructor of DbParams.

        :param host: the host name
        :param dbname: the database name
        :param user: the user name to use for the database
        :param password: the password of the user
        """
        self.host = host
        self.user = user
        self.dbname = dbname
        self.password = password

    def cnx_string(self):
        # type: () -> str
        """
        The connection string for the database.
        A string containing all the essentials data for a connection to a database.

        :return: the connection string
        """
        return 'postgresql://{u}:{pwd}@{h}/{dbname}'.format(
            h=self.host, u=self.user, dbname=self.dbname, pwd=self.password
        )

    def old_school_cnx_string(self):
        # type: () -> str
        """
        The connection string for the database (old school version).
        A string containg all the essentials data for a connection to a old school database (before Debian8). Old C++
        component does not support the classic "postgres://...".

        :return: the old school connection string
        """
        return "host={h} user={u} dbname={dbname} password={pwd}".format(
            h=self.host, u=self.user, dbname=self.dbname, pwd=self.password
        )


class DockerWrapper(object):
    """
    Class to launch a temporary docker with a PostgreSQL/PostGIS database.
    """

    def __init__(
        self,
        image_name,
        container_name=None,
        dockerfile_path=None,
        dbname='postgres',
        dbuser='docker',
        dbpassword='docker',
        env_vars={},
        mounts=None,
    ):
        # type: (str, Optional[str], Optional[str], str, str, str, Dict[str, str], Optional[List[docker.types.Mount]]) -> None
        """
        Constructor of PostgresDocker.
        """
        base_url = 'unix://var/run/docker.sock'
        self.docker_client = docker.DockerClient(base_url=base_url)
        self.docker_api_client = docker.APIClient(base_url=base_url)
        self.image_name = image_name
        self.dockerfile_path = dockerfile_path
        self.container_name = container_name
        self.dbname = dbname
        self.dbuser = dbuser
        self.dbpassword = dbpassword
        self.env_vars = env_vars
        self.mounts = mounts or []

        logger.info("Trying to build/update the docker image")

        try:
            if self.dockerfile_path:
                for build_output in self.docker_client.images.build(
                    path=self.dockerfile_path, tag=image_name, rm=True
                ):
                    logger.debug(build_output)
            else:
                self.docker_client.images.pull(image_name)

        except docker.errors.APIError as e:
            if e.is_server_error():
                logger.warning(
                    "[docker server error] A server error occcured, maybe missing internet connection?"
                )
                logger.warning("[docker server error] Details: {}".format(e))
                logger.warning(
                    "[docker server error] Checking if '{}' docker image is already built".format(image_name)
                )
                self.docker_client.images.get(image_name)
                logger.warning(
                    "[docker server error] Going on, as '{}' docker image is already built".format(image_name)
                )
            else:
                raise
        kwargs = {}
        # If tests are running in a container we need to attach to the same network
        network_name = os.getenv('NAVITIA_DOCKER_NETWORK')
        if network_name:
            kwargs['network'] = network_name
        self.container = self.docker_client.containers.create(
            image_name, name=self.container_name, environment=self.env_vars, mounts=self.mounts, **kwargs
        )
        logger.info("docker id is {}".format(self.container.id))
        logger.info("starting the temporary docker")
        self.container.start()
        container_info = self.docker_api_client.inspect_container(self.container.id)
        if not network_name:
            self.ip_addr = container_info.get('NetworkSettings', {}).get('IPAddress')
        else:
            self.ip_addr = (
                container_info.get('NetworkSettings', {})
                .get('Networks', {})
                .get(network_name, {})
                .get('IPAddress')
            )

        logging.debug('container inspect %s', container_info)

        if not self.ip_addr:
            logger.error("temporary docker {} not started".format(self.container.id))
            exit(1)

        # we poll to ensure that the database is ready
        self.test_db_cnx()

    def close(self):
        # type: () -> None
        """
        Terminate the Docker and clean it.
        """
        logger.info("stopping the temporary docker")
        self.container.stop()

        logger.info("removing the temporary docker")
        self.container.remove(v=True)

        # test to be sure the docker is removed at the end
        try:
            self.docker_client.containers.get(self.container.id)
        except docker.errors.NotFound:
            logger.info("the container is properly removed")
        else:
            logger.error("something is strange, the container is still there ...")
            exit(1)

    def get_db_params(self):
        # type: () -> DbParams
        """
        Create the connection parameters of the database.
        Default user and password are "docker" and default database is "postgres".

        :return: the DbParams for the database of the Docker
        """
        return DbParams(self.ip_addr, self.dbname, self.dbuser, self.dbpassword)

    @retry(stop_max_delay=10000, wait_fixed=100, retry_on_exception=lambda e: isinstance(e, Exception))
    def test_db_cnx(self):
        # type: () -> None
        """
        Test the connection to the database.
        """
        params = self.get_db_params()
        psycopg2.connect(database=params.dbname, user=params.user, password=params.password, host=params.host)


def PostgisDocker():
    return DockerWrapper(
        image_name='postgis:2.1',
        dockerfile_path='github.com/CanalTP/docker-postgis.git',
        dbname='postgres',
        dbuser='docker',
        dbpassword='docker',
    )


def PostgresDocker(db_name='tyr_test', user='postgres', pwd='postgres', mounts=None):
    env_vars = {'POSTGRES_DB': db_name, 'POSTGRES_USER': user, 'POSTGRES_PASSWORD': pwd}
    return DockerWrapper(
        image_name='postgres:9.4',
        container_name='tyr_test_postgres',
        dbname=env_vars['POSTGRES_DB'],
        dbuser=env_vars['POSTGRES_USER'],
        dbpassword=env_vars['POSTGRES_PASSWORD'],
        env_vars=env_vars,
        mounts=mounts,
    )
