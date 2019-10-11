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

from __future__ import absolute_import, print_function, division
from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import pytest
from navitiacommon import models
from tyr import app
from tyr import tasks


@pytest.fixture
def enable_mimir():
    previous_value = app.config['MIMIR_URL']
    app.config['MIMIR_URL'] = 'http://example.com'
    yield
    app.config['MIMIR_URL'] = previous_value


@pytest.fixture
def create_instance():
    with app.app_context():
        instance = models.Instance('fr')
        instance.import_ntfs_in_mimir = True
        models.db.session.add(instance)
        models.db.session.commit()
        return instance.id


def test_mimir_family_type_not_applicable(create_instance, enable_mimir):
    with app.app_context():
        instance = models.Instance.query.get(create_instance)
        actions = []
        actions.extend(tasks.send_to_mimir(instance, 'test.poi', 'dt'))
        assert actions == []


def test_mimir_family_type_poi(create_instance, enable_mimir):
    with app.app_context():
        instance = models.Instance.query.get(create_instance)
        actions = []
        actions.extend(tasks.send_to_mimir(instance, 'test.poi', 'poi'))
        assert actions[0].task == 'tyr.binarisation.poi2mimir'
        assert actions[0].args[1] == 'test.poi'
        assert len(actions) == 2  # poi2mimir + finish


def test_mimir_ntfs_false(create_instance, enable_mimir):
    with app.app_context():
        instance = models.Instance.query.get(create_instance)
        instance.import_ntfs_in_mimir = False
        actions = []
        actions.extend(tasks.send_to_mimir(instance, 'test.poi', 'poi'))
        assert actions == []


def test_mimir_family_type_poi_mimir_disabled(create_instance):
    with app.app_context():
        instance = models.Instance.query.get(create_instance)
        actions = []
        actions.extend(tasks.send_to_mimir(instance, 'test.poi', 'poi'))
        assert len(actions) == 0
