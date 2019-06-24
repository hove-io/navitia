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
from contextlib import contextmanager
import glob
import os
from navitiacommon import utils, launch_exec
import psycopg2
import zipfile
import logging

ALEMBIC_PATH_ED = os.environ.get('ALEMBIC_PATH', '../sql')
ALEMBIC_PATH_CITIES = os.environ.get('ALEMBIC_PATH_CITIES', '../cities')


@contextmanager
def cd(new_dir):
    """
    small helper to change the current dir
    """
    prev_dir = os.getcwd()
    os.chdir(os.path.expanduser(new_dir))
    try:
        yield
    finally:
        os.chdir(prev_dir)


def binarize(ed_db_params, output, ed_component_path, cities_db_params):
    logger = logging.getLogger(__name__)
    logger.info('creating data.nav')
    ed2nav = 'ed2nav'
    if ed_component_path:
        ed2nav = os.path.join(ed_component_path, ed2nav)
    launch_exec.launch_exec(
        ed2nav,
        [
            "-o",
            output,
            "--connection-string",
            ed_db_params.old_school_cnx_string(),
            "--cities-connection-string",
            cities_db_params.old_school_cnx_string(),
        ],
        logger,
    )
    logger.info("data.nav is created successfully: {}".format(output))


def import_data(data_dir, db_params, ed_component_path):
    """
    call the right component to import the data in the directory

    we loop through all files until we recognize one on them
    """
    log = logging.getLogger(__name__)
    files = glob.glob(data_dir + "/*")
    data_type, file_to_load = utils.type_of_data(files)
    if not data_type:
        log.info('unknown data type for dir {}, skipping'.format(data_dir))
        return

    # Note, we consider that we only have to load one kind of data per directory
    import_component = data_type + '2ed'
    if ed_component_path:
        import_component = os.path.join(ed_component_path, import_component)

    if file_to_load.endswith('.zip') or file_to_load.endswith('.geopal'):
        # TODO: handle geopal as non zip
        # if it's a zip, we unzip it
        zip_file = zipfile.ZipFile(file_to_load)
        zip_file.extractall(path=data_dir)
        file_to_load = data_dir

    if launch_exec.launch_exec(
        import_component, ["-i", file_to_load, "--connection-string", db_params.old_school_cnx_string()], log
    ):
        raise Exception('Error: problem with running {}, stoping'.format(import_component))


def load_cities(cities_file, cities_db_params, cities_exec_path):
    logger = logging.getLogger(__name__)

    cities_exec = os.path.join(cities_exec_path, 'cities')

    if launch_exec.launch_exec(
        cities_exec, ["-i", cities_file, "--connection-string", cities_db_params.old_school_cnx_string()], logger
    ):
        raise Exception('Error: problem with running {}, stoping'.format(cities_exec))


def load_data(data_dirs, ed_db_params, ed_component_path):
    logging.getLogger(__name__).info('loading {}'.format(data_dirs))

    for d in data_dirs:
        import_data(d, ed_db_params, ed_component_path)


def update_db(db_params, alembic_path):
    """
    enable postgis on the db and update it's scheme
    """
    cnx_string = db_params.cnx_string()

    # we need to enable postgis on the db
    cnx = psycopg2.connect(
        database=db_params.dbname, user=db_params.user, password=db_params.password, host=db_params.host
    )
    c = cnx.cursor()
    c.execute("create extension postgis;")
    c.close()
    cnx.commit()

    logging.getLogger(__name__).info('message = {}'.format(c.statusmessage))

    with cd(alembic_path):
        res = os.system('PYTHONPATH=. alembic -x dbname="{cnx}" upgrade head'.format(cnx=cnx_string))

        if res:
            raise Exception('problem with db update')


def generate_nav(
    data_dir, docker_ed, docker_cities, output_file, ed_component_path, cities_exec_path, import_cities
):
    """
    load all data either directly in data_dir if there is no sub dir, or all data in the subdir
    """
    cities_db_params = docker_cities.get_db_params()
    update_db(cities_db_params, ALEMBIC_PATH_CITIES)

    ed_db_params = docker_ed.get_db_params()
    update_db(ed_db_params, ALEMBIC_PATH_ED)

    if import_cities:
        if not os.path.exists(import_cities):
            raise Exception('Error: impossible to find {}, exiting'.format(import_cities))

        load_cities(import_cities, cities_db_params, cities_exec_path)

    if not os.path.exists(data_dir):
        raise Exception('Error: impossible to find {}, exiting'.format(data_dir))

    data_dirs = [
        os.path.join(data_dir, sub_dir_name)
        for sub_dir_name in os.listdir(data_dir)
        if os.path.isdir(os.path.join(data_dir, sub_dir_name))
    ] or [
        data_dir
    ]  # if there is no sub dir, we import only the files in the dir

    load_data(data_dirs, ed_db_params, ed_component_path)

    binarize(ed_db_params, output_file, ed_component_path, cities_db_params)
