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

# postgres/postgis image
POSTGIS_IMAGE = 'github.com/CanalTP/docker-postgis.git'
POSTGIS_CONTAINER_NAME = 'postgis:2.1'


class DbParams(object):
    def __init__(self, host, dbname, user, password):
        self.host = host
        self.user = user
        self.dbname = dbname
        self.password = password

    def cnx_string(self):
        return 'postgresql://{u}:{pwd}@{h}/{dbname}'.format(h=self.host,
                                                            u=self.user,
                                                            dbname=self.dbname,
                                                            pwd=self.password)

    def old_school_cnx_string(self):
        """
        old C++ component does not support the classic postgres://...

         refactor when moving to debian 8 as the classic cnx string will be recognized
        """
        return "host={h} user={u} dbname={dbname} password={pwd}".\
            format(h=self.host,
                   u=self.user,
                   dbname=self.dbname,
                   pwd=self.password)


class PostgresDocker(object):
    """
    launch a temporary docker with a postgresql-postgis db
    """
    def __init__(self):
        log = logging.getLogger(__name__)
        self.docker = docker.Client(base_url='unix://var/run/docker.sock')

        log.info("Trying to build/update the docker image")
        try:
            for build_output in self.docker.build(POSTGIS_IMAGE, tag=POSTGIS_CONTAINER_NAME, rm=True):
                log.debug(build_output)
        except docker.errors.APIError as e:
            if e.is_server_error():
                log.error("[docker server error] A server error occcured, maybe "
                                                "missing internet connection?")
                log.error("[docker server error] Details: {}".format(e))
                log.error("[docker server error] Trying to go on anyway, assuming '{}' docker image "
                                                "is already built...".format(POSTGIS_IMAGE))
            else:
                raise

        self.container_id = self.docker.create_container(POSTGIS_CONTAINER_NAME).get('Id')

        log.info("docker id is {}".format(self.container_id))

        log.info("starting the temporary docker")
        self.docker.start(self.container_id)
        self.ip_addr = self.docker.inspect_container(self.container_id).get('NetworkSettings', {}).get('IPAddress')

        if not self.ip_addr:
            log.error("temporary docker {} not started".format(self.container_id))
            exit(1)

        # we poll to ensure that the db is ready
        self.test_db_cnx()

    def __enter__(self):
        return self

    def __exit__(self, *args, **kwargs):
        logging.getLogger(__name__).info("stoping the temporary docker")
        self.docker.stop(container=self.container_id)

        logging.getLogger(__name__).info("removing the temporary docker")
        self.docker.remove_container(container=self.container_id, v=True)

        # test to be sure the docker is removed at the end
        for cont in self.docker.containers(all=True):
            if cont['Image'].split(':')[0] == POSTGIS_IMAGE:
                if self.container_id in (name[1:] for name in cont['Names']):
                    logging.getLogger(__name__).error("something is strange, the container is still there ...")
                    exit(1)

    def get_db_params(self):
        """
        cnx param to the database
        default user and password are 'docker' and default db is postgres
        """
        return DbParams(self.ip_addr, 'postgres', 'docker', 'docker')

    @retry(stop_max_delay=10000, wait_fixed=100,
           retry_on_exception=lambda e: isinstance(e, Exception))
    def test_db_cnx(self):
        psycopg2.connect(
            database=self.get_db_params().dbname,
            user=self.get_db_params().user,
            password=self.get_db_params().password,
            host=self.get_db_params().host
        )
