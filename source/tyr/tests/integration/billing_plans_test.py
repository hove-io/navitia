# coding: utf-8
# Copyright (c) 2001-2018, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
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

from __future__ import absolute_import, print_function, division
from tests.check_utils import api_get, api_post, api_put
import json


def get_by_name(resp, name):
    for r in resp:
        if r['name'] == name:
            return r

    return None


def test_basic_billing_plans():
    """
    Tests on api billing_plans
    """
    resp = api_get('/v0/billing_plans/')
    assert len(resp) == 5

    # verifying all attributes of one element
    bp = get_by_name(resp, 'sncf_dev')
    # block able and notify only once for sncf with max_request_count > 0
    assert bp is not None
    assert bp['name'] == 'sncf_dev'
    assert bp['lockable'] is True
    assert bp['max_request_count'] == 3000
    assert bp['max_object_count'] == 60000
    assert bp['notify_threshold_list'] == [100]
    assert bp['default'] is True
    assert bp['created_at'] is not None
    assert bp['updated_at'] is None
    assert bp['id'] == 4
    assert bp['end_point']['name'] == 'sncf'

    # neither block able nor notify for all end_points with max_request_count 0
    bp = get_by_name(resp, 'sncf_ent')
    assert bp['name'] == 'sncf_ent'
    assert bp['lockable'] is False
    assert bp['notify_threshold_list'] == []
    assert bp['max_request_count'] == 0
    assert bp['end_point']['name'] == 'sncf'
    bp = get_by_name(resp, 'nav_ctp')
    assert bp['name'] == 'nav_ctp'
    assert bp['lockable'] is False
    assert bp['notify_threshold_list'] == []
    assert bp['max_request_count'] == 0
    assert bp['end_point']['name'] == 'navitia.io'
    bp = get_by_name(resp, 'nav_ent')
    assert bp['name'] == 'nav_ent'
    assert bp['lockable'] is False
    assert bp['notify_threshold_list'] == []
    assert bp['max_request_count'] == 0
    assert bp['end_point']['name'] == 'navitia.io'

    # not block able but notify two times for navitia.io with max_request_count > 0
    bp = get_by_name(resp, 'nav_dev')
    assert bp['name'] == 'nav_dev'
    assert bp['lockable'] is False
    assert bp['notify_threshold_list'] == [75, 100]
    assert bp['max_request_count'] == 3000
    assert bp['end_point']['name'] == 'navitia.io'


def test_actions_on_billing_plans():
    """
    Tests actions on api billing_plans
    """
    billing_plan = {
        'name': 'bp_test_1',
        'max_request_count': 100,
        'max_object_count': 1000,
        'default': False,
        'lockable': True,
        'notify_threshold_list': [55, 77, 100],
    }
    data = json.dumps(billing_plan)
    resp = api_post('/v0/billing_plans/', data=data, content_type='application/json')
    assert resp['name'] == 'bp_test_1'
    assert resp['max_request_count'] == 100
    assert resp['end_point']['name'] == 'navitia.io'
    assert resp['lockable'] is True
    assert resp['default'] is False
    assert resp['notify_threshold_list'] == [55, 77, 100]
    # since end_point is absent in the post data, default end_point is used.
    assert resp['end_point']['name'] == 'navitia.io'
    assert resp['end_point']['default'] is True
    assert resp['id'] == 6

    # Modify certain attributes of the existing billing_plan
    # Will update only modified attributes.
    billing_plan = {'max_request_count': 101, 'lockable': True, 'notify_threshold_list': [75, 100]}
    data = json.dumps(billing_plan)

    # Put without id
    resp, status = api_put(
        '/v0/billing_plans/', check=False, data=json.dumps(data), content_type='application/json'
    )
    assert resp['status'] == 'error'
    assert resp['message'] == 'billing_plan_id is required'
    assert status == 400

    # Put with id absent
    resp, status = api_put(
        '/v0/billing_plans/100', check=False, data=json.dumps(data), content_type='application/json'
    )
    assert status == 404

    # Put with id
    resp = api_put('/v0/billing_plans/6', data=data, content_type='application/json')
    assert resp['name'] == 'bp_test_1'
    assert resp['max_request_count'] == 101
    assert resp['end_point']['name'] == 'navitia.io'
    assert resp['lockable'] is True
    assert resp['default'] is False
    assert resp['notify_threshold_list'] == [75, 100]
    assert resp['end_point']['name'] == 'navitia.io'

    # Get all billing_plans
    resp = api_get('/v0/billing_plans/')
    assert len(resp) == 6

    # Put without lockable and notify_threshold_list should not modify existing values
    # notify_threshold_list = [75, 100] and lockable = True
    billing_plan = {'max_request_count': 101}
    data = json.dumps(billing_plan)
    resp = api_put('/v0/billing_plans/6', data=data, content_type='application/json')
    assert resp['name'] == 'bp_test_1'
    assert resp['max_request_count'] == 101
    assert resp['end_point']['name'] == 'navitia.io'
    assert resp['lockable'] is True
    assert resp['notify_threshold_list'] == [75, 100]
    assert resp['end_point']['name'] == 'navitia.io'
