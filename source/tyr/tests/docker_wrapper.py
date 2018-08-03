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

import docker
import logging
from retrying import retry
import psycopg2

# postgres image
POSTGRES_IMAGE = 'postgres'
POSTGRES_CONTAINER_NAME = 'tyr_test_postgres'


def _get_docker_file():
    """
    Return a dumb DockerFile
    The best way to get the image would be to get postgres:9.4 it from dockerhub,
    but with this dumb wrapper the runtime time of the unit tests
    is reduced by 10s
    """
    from io import BytesIO
    return BytesIO(str('FROM {}'.format(POSTGRES_IMAGE)))

class PostgresDocker(object):
    USER = 'postgres'
    PWD = 'postgres'
    DBNAME = 'tyr_test'
    """
    launch a temporary docker with a postgresql db
    """
    def __init__(self, user=USER, pwd=PWD, dbname=DBNAME):
        log = logging.getLogger(__name__)
        base_url = 'unix://var/run/docker.sock'
        self.docker_client = docker.DockerClient(base_url=base_url)
        self.docker_api_client = docker.APIClient(base_url=base_url)

        log.info('Trying to build/update the docker image')
        try:
            for build_output in self.docker_client.images.build(fileobj=_get_docker_file(), tag=POSTGRES_IMAGE, rm=True):
                log.debug(build_output)
        except docker.errors.APIError as e:
            if e.is_server_error():
                log.warn("[docker server error] A server error occcured, maybe "
                                                "missing internet connection?")
                log.warn("[docker server error] Details: {}".format(e))
                log.warn("[docker server error] Checking if '{}' docker image "
                                               "is already built".format(POSTGRES_IMAGE))
                self.docker_client.images.get(POSTGRES_IMAGE)
                log.warn("[docker server error] Going on, as '{}' docker image "
                                               "is already built".format(POSTGRES_IMAGE))
            else:
                raise

        self.container = self.docker_client.containers.create(POSTGRES_IMAGE, name=POSTGRES_CONTAINER_NAME)

        log.info("docker id is {}".format(self.container.id))

        log.info("starting the temporary docker")
        self.container.start()
        self.ip_addr = self.docker_api_client.inspect_container(self.container.id).get('NetworkSettings', {}).get('IPAddress')

        if not self.ip_addr:
            log.error("temporary docker {} not started".format(self.container.id))
            assert False

        # we create an empty database to prepare for the test
        self._create_db(user, pwd, dbname)

    def __enter__(self):
        return self

    def __exit__(self, *args, **kwargs):
        logging.getLogger(__name__).info("stopping the temporary docker")
        self.container.stop()

        logging.getLogger(__name__).info("removing the temporary docker")
        self.container.remove(v=True)

        # test to be sure the docker is removed at the end
        still_exists = True
        try:
            self.docker_client.containers.get(self.container.id)
        except docker.errors.NotFound:
            still_exists = False

        if still_exists:
            logging.getLogger(__name__).error("something is strange, the container is still there ...")
            exit(1)

    @retry(stop_max_delay=10000, wait_fixed=100,
           retry_on_exception=lambda e: isinstance(e, Exception))
    def _create_db(self, user, pwd, dbname):
        connect = psycopg2.connect(user=user, host=self.ip_addr, password=pwd)
        connect.set_isolation_level(psycopg2.extensions.ISOLATION_LEVEL_AUTOCOMMIT)
        cur = connect.cursor()
        cur.execute('CREATE DATABASE ' + dbname)
        cur.close()
        connect.close()

