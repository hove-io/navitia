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
import retrying

POSTGIS_IMAGE = 'github.com/helmi03/docker-postgis.git'
POSTGIS_CONTAINER_NAME = 'postgis:2.1'

import psycopg2

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


class PostgresDocker(object):
    """
    launch a temporary docker with a postgresql db
    """
    def __init__(self):
        self.docker_client = docker.Client(base_url='unix://var/run/docker.sock')

        logging.getLogger(__name__).info("building the temporary docker image")
        self.docker_client.build(POSTGIS_IMAGE, tag=POSTGIS_CONTAINER_NAME, rm=True)

        self.docker_container = self.docker_client.create_container(POSTGIS_CONTAINER_NAME).get('Id')

        logging.getLogger(__name__).info("docker id is {}".format(self.docker_container))

        logging.getLogger(__name__).info("starting the temporary docker")
        self.docker_client.start(self.docker_container)
        self.ip_addr = self.docker_client.inspect_container(self.docker_container).get('NetworkSettings', {}).get('IPAddress')

        if not self.ip_addr:
            logging.getLogger(__name__).error("temporary docker {} not started".format(self.docker_container))
            exit(1)

        # we poll to ensure that the db is ready
        def db_cnx():
            psycopg2.connect(self.get_db_params().cnx_string())

        retrying.Retrying(stop_max_delay=10000,
                          wait_fixed=100,
                          retry_on_exception=lambda e: isinstance(e, Exception)).call(db_cnx)

    def __enter__(self):
        return self

    def __exit__(self, *args, **kwargs):
        logging.getLogger(__name__).info("stoping the temporary docker")
        self.docker_client.stop(container=self.docker_container)

        logging.getLogger(__name__).info("removing the temporary docker")
        self.docker_client.remove_container(container=self.docker_container)

        # test to be sure the docker is removed at the end
        for cont in self.docker_client.containers(all=True):
            if cont['Image'].split(':')[0] == POSTGIS_IMAGE:
                if self.docker_container in (name[1:] for name in cont['Names']):
                    logging.getLogger(__name__).error("something is strange, the container is still there ...")
                    exit(1)

    def get_db_params(self):
        return DbParams(self.ip_addr, 'postgres', 'docker', 'docker')
