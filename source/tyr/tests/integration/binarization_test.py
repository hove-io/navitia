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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals
import os
import shutil
import glob
import pytest

from tyr import app
from navitiacommon import models

from tyr.tasks import create_actions


@pytest.yield_fixture(scope="function", autouse=True)
def create_instance():
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        models.db.session.commit()

        yield instance

        models.db.session.delete(instance)


@pytest.mark.usefixtures("init_instances_dir")
def test_actions_according_to_data(create_instance):
    # Init Test
    source_dir = glob.glob("/tmp/instance*/source*/")[0]
    # os.mkdir(os.path.join(source_dir, "fusio"))
    osm_dir_path = os.path.join(source_dir, "osm")
    os.mkdir(osm_dir_path)
    shutil.copy(
        os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/empty_pbf.osm.pbf'
        ),
        osm_dir_path,
    )

    # Only OSM file on source so expected actions are 'osm2ed', 'ed2nav'
    actions = create_actions(
        files=glob.glob(source_dir + "/*"),
        instance=create_instance,
        backup_file=False,
        reload=False,
        custom_output_dir=None,
        skip_mimir=False,
    )
    assert len(actions) == 4
    assert "osm2ed" in actions[0].task
    assert "finish_job" in actions[1].task
    assert "ed2nav" in actions[2].tasks[0].task
    assert "finish_job" in actions[3].task
