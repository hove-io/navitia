# coding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, division, unicode_literals
from tests.check_utils import api_get, api_post, api_delete, api_put
import json
import pytest
from navitiacommon import models
from tyr import app


@pytest.fixture
def create_instance():
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        models.db.session.commit()

        return instance.id


@pytest.fixture
def create_poi_type_json_obj(poi_types_json):
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        poi_type_json_obj = models.PoiTypeJson(json.dumps(poi_types_json), instance)
        models.db.session.add(poi_type_json_obj)
        models.db.session.commit()

        return poi_type_json_obj


def check_traveler_profile(profile, params):
    for key, param in params.iteritems():
        assert profile[key] == param


@pytest.fixture
def poi_types_json():
    return {
        "poi_types": [
            {"id": "pdv", "name": "Point de vente"},
            {"id": "mairie", "name": u"Hôtel de ville"},
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "amenity:parking", "name": "Parking"},
        ],
        "rules": [
            {
                "osm_tags_filters": [
                    {"key": "amenity:park", "value": "yes"},
                    {"key": "amenity", "value": "shop"},
                ],
                "poi_type_id": "pdv",
            },
            {"osm_tags_filters": [{"key": "amenity", "value": "city_hall"}], "poi_type_id": "mairie"},
            {"osm_tags_filters": [{"key": "amenity:shop", "value": "ticket"}], "poi_type_id": "pdv"},
        ],
    }


def test_post_and_get_poi_type_json(create_instance, poi_types_json):
    resp = api_post(
        '/v0/instances/fr/poi_types', data=json.dumps(poi_types_json), content_type='application/json'
    )
    assert resp == poi_types_json

    wrong_poi_types_json = {"poi_types": []}
    resp, status_code = api_post(
        '/v0/instances/fr/poi_types',
        data=json.dumps(wrong_poi_types_json),
        content_type='application/json',
        check=False,
    )
    assert status_code == 400

    # check special characters correctly handled
    with app.app_context():
        ptp = models.PoiTypeJson.query.all()
        assert u'Hôtel de ville' in ptp[0].poi_types_json


def test_get_poi_type_json(create_poi_type_json_obj, poi_types_json):
    resp = api_get('/v0/instances/fr/poi_types')

    assert resp == poi_types_json


def test_put_poi_type_json(create_poi_type_json_obj):
    new_poi_types_json = {
        "poi_types": [
            {"id": "parking", "name": "Parking"},
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "amenity:parking", "name": "Parking"},
        ],
        "rules": [{"osm_tags_filters": [{"key": "amenity:park", "value": "yes"}], "poi_type_id": "parking"}],
    }

    resp, status_code = api_put(
        '/v0/instances/fr/poi_types',
        data=json.dumps(new_poi_types_json),
        content_type='application/json',
        check=False,
    )

    assert status_code == 405


def test_delete_poi_type_json(create_poi_type_json_obj):
    resp, status_code = api_delete('/v0/instances/fr/poi_types', check=False, no_json=True)
    assert resp.decode("utf-8") == ''
    assert status_code == 204
