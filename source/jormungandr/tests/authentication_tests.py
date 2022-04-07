# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
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
from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr.tests.utils_test import user_set, FakeUser
from navitiacommon import models
import requests_mock

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *
from jormungandr import app
import json
from navitiacommon.constants import DEFAULT_SHAPE_SCOPE
from datetime import datetime


BRAGI_RESPONSE = {"features": []}


def check_dataset(m, datasets):
    params = m.request_history[-1].qs
    assert set(datasets) == set(params.get('pt_dataset[]'))


authorizations = {
    'bob': {
        "main_routing_test": {'ALL': True},
        "departure_board_test": {'ALL': False},
        "empty_routing_test": {'ALL': False},
    },
    'bobette': {
        # bobette cannot access anything
        "main_routing_test": {'ALL': False},
        "departure_board_test": {'ALL': False},
        "empty_routing_test": {'ALL': False},
    },
    'bobitto': {
        # bobitto can access all since empty_routing_test is free
        "main_routing_test": {'ALL': True},
        "departure_board_test": {'ALL': True},
        "empty_routing_test": {'ALL': False},
    },
    'tgv': {
        # tgv can only access main_routing_test
        "main_routing_test": {'ALL': True},
        "departure_board_test": {'ALL': False},
        "empty_routing_test": {'ALL': False},
    },
    'test_user_blocked': {
        "main_routing_test": {'ALL': True},
        "departure_board_test": {'ALL': True},
        "empty_routing_test": {'ALL': True},
    },
    'test_user_not_blocked': {
        "main_routing_test": {'ALL': True},
        "departure_board_test": {'ALL': True},
        "empty_routing_test": {'ALL': True},
    },
    'user_without_any_coverage': {
        "main_routing_test": {'ALL': False},
        "departure_board_test": {'ALL': False},
        "empty_routing_test": {'ALL': False},
    },
}


class FakeUserAuth(FakeUser):
    @classmethod
    def get_from_token(cls, token, valid_until):
        """
        Create an empty user
        """
        return user_in_db_auth[token]

    def has_access(self, instance_name, api_name):
        """
        This is made to avoid using of database
        """
        if self.is_super_user:
            return True
        return authorizations[self.login][instance_name][api_name]

    def _get_all_explicitly_authorized_instances(self):
        """mocked to avoid using the db"""
        authorized_instance_names = [i for i, a in authorizations[self.login].items() if a['ALL']]
        return [mock_instances[n] for n in authorized_instance_names]

    def _get_all_free_instances(self):
        """mocked to avoid using the db"""
        return [i for i in mock_instances.values() if i.is_free]

    def _get_all_instances(self):
        return mock_instances.values()


multipolygon = (
    '{"type":"Feature","geometry":{"type":"MultiPolygo","coordiates": '
    '[[[[2.4,48.6],[2.8,48.6],[2.7,48.9],[2.4,48.6]]], [[[2.1,48.9],[2.2,48.6],[2.4,48.9],[2.1,48.9]]]]}}'
)

user_in_db_auth = {
    'bob': FakeUserAuth('bob', 1),
    'bobette': FakeUserAuth('bobette', 2),
    'bobitto': FakeUserAuth('bobitto', 3),
    'tgv': FakeUserAuth('tgv', 4, have_access_to_free_instances=False),
    'test_user_blocked': FakeUserAuth('test_user_blocked', 5, True, False, True),
    'test_user_not_blocked': FakeUserAuth('test_user_not_blocked', 6, True, False, False),
    'super_user': FakeUserAuth('super_user', 7, is_super_user=True),
    'user_without_any_coverage': FakeUserAuth(
        'user_without_any_coverage', 8, have_access_to_free_instances=False
    ),
    "api_users": FakeUserAuth(
        "api_users",
        9,
        shape=multipolygon,
        shape_scope=["poi", "admin"],
        default_coord="2.4;48.6",
        block_until=datetime(year=2019, month=10, day=5, hour=23, minute=58, second=30),
    ),
}


class FakeInstance(models.Instance):
    def __init__(self, name, is_free):
        self.name = name
        self.is_free = is_free
        self.id = name

    @classmethod
    def get_by_name(cls, name):
        return mock_instances.get(name)


mock_instances = {
    'main_routing_test': FakeInstance('main_routing_test', is_free=False),
    'departure_board_test': FakeInstance('departure_board_test', is_free=False),
    'empty_routing_test': FakeInstance('empty_routing_test', is_free=True),
}


class AbstractTestAuthentication(AbstractTestFixture):
    def setUp(self):
        self.old_public_val = app.config['PUBLIC']
        app.config['PUBLIC'] = False
        self.old_db_val = app.config['DISABLE_DATABASE']
        app.config['DISABLE_DATABASE'] = False
        self.app = app.test_client()

        self.old_instance_getter = models.Instance.get_by_name
        models.Instance.get_by_name = FakeInstance.get_by_name

    def tearDown(self):
        app.config['PUBLIC'] = self.old_public_val
        app.config['DISABLE_DATABASE'] = self.old_db_val
        models.Instance.get_by_name = self.old_instance_getter


@dataset({"main_routing_test": {}})
class TestUsersApi(AbstractTestAuthentication):
    def test_users_default_values(self):
        """
        User With default values
        """
        with user_set(app, FakeUserAuth, 'bob'):
            response_obj = self.app.get('/v1/users')
            response = json.loads(response_obj.data)
            assert response["type"] == "with_free_instances"
            assert "shape_scope" in response
            assert len(DEFAULT_SHAPE_SCOPE) == len(response['shape_scope'])
            for ss in response['shape_scope']:
                assert ss in DEFAULT_SHAPE_SCOPE

    def test_users_with_shape(self):
        """
        User with shape and shape_scope
        """
        with user_set(app, FakeUserAuth, 'api_users'):
            response_obj = self.app.get('/v1/users')
            response = json.loads(response_obj.data)
            assert response["type"] == "with_free_instances"
            assert len(response['shape_scope']) == 2
            for ss in response['shape_scope']:
                assert ss in ["poi", "admin"]
            assert "shape" in response
            assert "coord" in response
            assert response["coord"]["lon"] == "2.4"
            assert response["coord"]["lat"] == "48.6"
            assert response["block_until"] == "20191005T235830"

    def test_users_without_token(self):
        """
        User without token
        """
        response_obj = self.app.get('/v1/users')
        assert response_obj.status_code == 401


@dataset({"main_routing_test": {}, "departure_board_test": {}})
class TestBasicAuthentication(AbstractTestAuthentication):
    def test_coverage(self):
        """
        User only has access to the first region
        """
        with user_set(app, FakeUserAuth, 'bob'):
            response_obj = self.app.get('/v1/coverage')
            response = json.loads(response_obj.data)
            assert 'regions' in response
            assert len(response['regions']) == 1
            assert response['regions'][0]['id'] == "main_routing_test"

    def test_auth_required(self):
        """
        if no token is given we are asked to log in (code 401) and a challenge is sent (header WWW-Authenticate)
        """
        response_obj = self.app.get('/v1/coverage')
        assert response_obj.status_code == 401
        assert 'WWW-Authenticate' in response_obj.headers

    def test_status_code(self):
        """
        We query the api with user 1 who have access to the main routing test and not to the departure board
        """
        requests_status_codes = [
            ('/v1/coverage/main_routing_test', 200),
            ('/v1/coverage/departure_board_test', 403),
            # stopA and stopB and in main routing test, all is ok
            ('/v1/journeys?from=stopA&to=stopB&datetime=20120614T080000', 200),
            # stop1 is in departure board -> KO
            ('/v1/journeys?from=stopA&to=stop2&datetime=20120614T080000', 403),
            # stop1 and stop2 are in departure board -> KO
            ('/v1/journeys?from=stop1&to=stop2&datetime=20120614T080000', 403),
        ]

        with user_set(app, FakeUserAuth, 'bob'):
            for request, status_code in requests_status_codes:
                assert self.app.get(request).status_code == status_code

    def test_unkown_region(self):
        """
        the authentication process must not mess if the region is not found
        """
        with user_set(app, FakeUserAuth, 'bob'):
            r, status = self.query_no_assert('/v1/coverage/the_marvelous_unknown_region/stop_areas')

            assert status == 404
            assert 'error' in r
            assert (
                get_not_null(r, 'error')['message'] == "The region the_marvelous_unknown_region doesn't exists"
            )


@dataset({"main_routing_test": {}})
class TestIfUserIsBlocked(AbstractTestAuthentication):
    def test_status_code(self):
        """
        We query the api with user 5 who must be blocked
        """
        requests_status_codes = [
            ('/v1/coverage/main_routing_test', 429),
            ('/v1/coverage/departure_board_test', 429),
        ]

        with user_set(app, FakeUserAuth, 'test_user_blocked'):
            for request, status_code in requests_status_codes:
                assert self.app.get(request).status_code == status_code


@dataset({"main_routing_test": {}})
class TestIfUserIsNotBlocked(AbstractTestAuthentication):
    def test_status_code(self):
        """
        We query the api with user 6 who must not be blocked
        """
        requests_status_codes = [('/v1/coverage/main_routing_test', 200)]

        with user_set(app, FakeUserAuth, 'test_user_not_blocked'):
            for request, status_code in requests_status_codes:
                assert self.app.get(request).status_code == status_code


# Coverage 'empty_routing_test' is set to free so that 'main_routing_test' is chosen as best coverage
@dataset(
    {"main_routing_test": {}, "departure_board_test": {}, "empty_routing_test": {'is_free': True}},
    global_config={'activate_bragi': True},
)
class TestOverlappingAuthentication(AbstractTestAuthentication):
    def test_coverage(self):
        with user_set(app, FakeUserAuth, 'bobitto'):
            response = self.query('v1/coverage')

            r = get_not_null(response, 'regions')

            region_ids = {region['id']: region for region in r}
            assert len(region_ids) == 3

            assert 'main_routing_test' in region_ids
            assert 'departure_board_test' in region_ids
            # bob have not the access to this region, but the region is free so it is there anyway
            assert 'empty_routing_test' in region_ids

        with user_set(app, FakeUserAuth, 'bobette'):
            response = self.query('v1/coverage')
            r = get_not_null(response, 'regions')

            region_ids = {region['id']: region for region in r}
            assert len(region_ids) == 1
            # bobette does not have access to anything, so we only have the free region here
            assert 'empty_routing_test' in region_ids

        with user_set(app, FakeUserAuth, 'tgv'):
            response = self.query('v1/coverage')
            r = get_not_null(response, 'regions')

            region_ids = {region['id']: region for region in r}
            assert len(region_ids) == 1
            # tgv must not see free regions
            assert 'empty_routing_test' not in region_ids

    def test_pt_ref_for_bobitto(self):
        with user_set(app, FakeUserAuth, 'bobitto'):
            response = self.query('v1/coverage/main_routing_test/stop_points')
            assert 'error' not in response
            response = self.query('v1/coverage/departure_board_test/stop_points')
            assert 'error' not in response

            # the empty region is empty, so no stop points. but we check that we have no authentication errors
            response, status = self.query_no_assert('v1/coverage/empty_routing_test/stop_points')
            assert status == 404
            assert 'error' in response
            assert 'unknown_object' in response['error']['id']

    def test_pt_ref_for_bobette(self):
        with user_set(app, FakeUserAuth, 'bobette'):
            _, status = self.query_no_assert('v1/coverage/main_routing_test/stop_points')
            assert status == 403
            _, status = self.query_no_assert('v1/coverage/departure_board_test/stop_points')
            assert status == 403

            _, status = self.query_no_assert('v1/coverage/empty_routing_test/stop_points')
            assert status == 404  # same than for bobitto, we have access to the region, but no stops

    def test_stop_schedules_for_bobette(self):
        with user_set(app, FakeUserAuth, 'bobette'):
            _, status = self.query_no_assert('v1/coverage/main_routing_test/stop_areas/stopA/stop_schedules')
            assert status == 403
            _, status = self.query_no_assert('v1/coverage/departure_board_test/stop_areas/stop1/stop_schedules')
            assert status == 403
            # we get a 404 (because 'stopbidon' cannot be found) and not a 403
            _, status = self.query_no_assert(
                'v1/coverage/empty_routing_test/stop_areas/' 'stopbidon/stop_schedules'
            )
            assert status == 404

    def test_stop_schedules_for_tgv(self):
        with user_set(app, FakeUserAuth, 'tgv'):
            response = self.query(
                'v1/coverage/main_routing_test/stop_areas/stopA/stop_schedules?from_datetime=20120614T080000'
            )
            assert 'error' not in response
            _, status = self.query_no_assert('v1/coverage/departure_board_test/stop_areas/stop1/stop_schedules')
            assert status == 403
            _, status = self.query_no_assert(
                'v1/coverage/empty_routing_test/stop_areas/' 'stopbidon/stop_schedules'
            )
            assert status == 403

    def test_stop_schedules_for_bobitto(self):
        with user_set(app, FakeUserAuth, 'bobitto'):
            response = self.query(
                'v1/coverage/main_routing_test/stop_areas/' 'stopA/stop_schedules?from_datetime=20120614T080000'
            )
            assert 'error' not in response
            response = self.query(
                'v1/coverage/departure_board_test/stop_areas/'
                'stop1/stop_schedules?from_datetime=20120614T080000'
            )
            assert 'error' not in response

            _, status = self.query_no_assert(
                'v1/coverage/empty_routing_test/stop_areas/'
                'stopbidon/stop_schedules?from_datetime=20120614T080000'
            )
            assert status == 404

    def test_journeys_for_bobitto(self):
        with user_set(app, FakeUserAuth, 'bobitto'):
            response = self.query('/v1/journeys?from=stopA&to=stopB&datetime=20120614T080000')
            assert 'error' not in response
            response = self.query('/v1/journeys?from=stop1&to=stop2&datetime=20120614T080000')
            assert 'error' not in response

    def test_journeys_for_tgv(self):
        with user_set(app, FakeUserAuth, 'tgv'):
            response = self.query('/v1/journeys?from=stopA&to=stopB&datetime=20120614T080000')
            assert 'error' not in response
            _, status = self.query_no_assert('/v1/journeys?from=stop1&to=stop2&datetime=20120614T080000')
            assert status == 403

            _, status = self.query_no_assert(
                '/v1/coverage/empty_routing_test/journeys?from=stop1&to=stop2&datetime=20120614T080000'
            )
            assert status == 403

    def test_wrong_journeys_for_bobitto(self):
        """
        we query with one stop for main_routing and one from departure_board,
        we have access to both, but we get an error
        """
        with user_set(app, FakeUserAuth, 'bobitto'):
            response, status = self.query_no_assert('/v1/journeys?from=stopA&to=stop2&datetime=20120614T080000')
            assert status == 404
            assert 'error' in response and response['error']['id'] == "unknown_object"

    def test_journeys_for_bobette(self):
        """
        bobette have access only to the empty region which overlap the main routing test
        so by stops no access to any region
        but by coord she can query the empty_routing_test region
        """
        with user_set(app, FakeUserAuth, 'bobette'):
            response, status = self.query_no_assert('/v1/journeys?from=stopA&to=stopB&datetime=20120614T080000')
            assert status == 403
            response, status = self.query_no_assert('/v1/journeys?from=stop1&to=stop2&datetime=20120614T080000')
            assert status == 403

            response, status = self.query_no_assert(
                '/v1/journeys?from={from_coord}&to={to_coord}&datetime={d}'.format(
                    from_coord=s_coord, to_coord=r_coord, d='20120614T08'
                )
            )
            assert 'error' in response
            assert response['error']['id'] == "no_origin_nor_destination"
            assert status == 404

    def test_places_for_bobitto(self):
        with user_set(app, FakeUserAuth, "bobitto"):
            response = self.query('v1/coverage/main_routing_test/places?q=toto')
            assert 'error' not in response
            response = self.query('v1/coverage/departure_board_test/places?q=toto')
            assert 'error' not in response
            response = self.query('v1/coverage/empty_routing_test/places?q=toto')
            assert 'error' not in response
            # this test suppose no elasticsearch is lanched at localhost
            _, status = self.query_no_assert('v1/places?q=toto')
            assert status == 500

    def test_places_for_bobette(self):
        with user_set(app, FakeUserAuth, "bobette"):
            _, status = self.query_no_assert('v1/coverage/main_routing_test/places?q=toto')
            assert status == 403
            _, status = self.query_no_assert('v1/coverage/departure_board_test/places?q=toto')
            assert status == 403
            response = self.query('v1/coverage/empty_routing_test/places?q=toto')
            assert 'error' not in response
            # this test suppose no elasticsearch is lanched at localhost
            _, status = self.query_no_assert('v1/places?q=toto')
            assert status == 500

    def test_places_for_tgv(self):
        with user_set(app, FakeUserAuth, "tgv"):
            response = self.query('v1/coverage/main_routing_test/places?q=toto')
            assert 'error' not in response
            _, status = self.query_no_assert('v1/coverage/departure_board_test/places?q=toto')
            assert status == 403
            _, status = self.query_no_assert('v1/coverage/empty_routing_test/places?q=toto')
            assert status == 403

    def test_sort_coverage(self):
        with user_set(app, FakeUserAuth, 'bobitto'):
            response = self.query('v1/coverage')

            regions = get_not_null(response, 'regions')
            assert len(regions) == 3
            assert regions[0]["name"] == 'departure board'
            assert regions[1]["name"] == 'empty routing'
            assert regions[2]["name"] == 'routing api data'

    def test_coverage_by_coords(self):
        """
        Test coverage containing coordinates, bobitto user authorized
        """
        with user_set(app, FakeUserAuth, 'bobitto'):
            response = self.query('v1/coverage/{coords}'.format(coords=s_coord))

            r = get_not_null(response, 'regions')

            assert {region['id'] for region in r} == {'main_routing_test'}

    def test_auth_required_coords(self):
        """
        Test coverage containing coordinates, without user
        """
        response_obj = self.app.get('/v1/coverage/{coords}'.format(coords=s_coord))
        assert response_obj.status_code == 401
        assert 'WWW-Authenticate' in response_obj.headers

    def test_status_code_coords(self):
        """
        Test of status response for coverage containing coordinates
        """
        with user_set(app, FakeUserAuth, 'bob'):
            # Coverage by coords
            response = self.app.get('/v1/coverage/{coords}'.format(coords=s_coord))
            assert response.status_code == 200

    def test_unkown_region_coords(self):
        """
        Test coverage containing coordinates, coordinate outside region
        """
        with user_set(app, FakeUserAuth, 'bob'):
            # coords outside regions
            r, status = self.query_no_assert('/v1/coverage/{lon};{lat}/stop_areas'.format(lon=20, lat=15))
            assert status == 404
            assert 'error' in r
            assert get_not_null(r, 'error')['message'] == "No region available for the coordinates:20.0, 15.0"

    def test_pt_ref_for_bobitto_coords(self):
        """
        Test ptref for coverage containing coordinates
        """
        with user_set(app, FakeUserAuth, 'bobitto'):
            # By coords: All stops of main_routing_test coverage
            response = self.query('v1/coverage/{coords}/stop_points'.format(coords=s_coord))
            stop_points = get_not_null(response, 'stop_points')
            assert len(stop_points) == 4

            response = self.query(
                'v1/coverage/{coords}/stop_points/{id}'.format(coords=s_coord, id='stop_point:stopB')
            )
            stop_points = get_not_null(response, 'stop_points')
            assert len(stop_points) == 1
            assert stop_points[0]['id'] == 'stop_point:stopB'

    def test_places_authentication(self):
        """
        test the v1/places authentication

        Only users with open data access can use this API and the user's authentication drives the underlying
        query made to bragi (the external autocomplete service).

        navitia gives bragi all the instances the user can use
        """
        response = {"features": []}
        # bob is a normal user, it can access the open_data, and it can access main_routing_test
        # he thus can use main_routing_test and empty_routing_test (because it's opendata)
        with user_set(app, FakeUserAuth, 'bob'):
            with requests_mock.Mocker() as m:
                m.get(requests_mock.ANY, json=response)
                r, status = self.query_no_assert('/v1/places?q=bob')
                assert status == 200
                assert m.called
                check_dataset(m, {'main_routing_test', 'empty_routing_test'})

        # user_without_any_coverage cannot access anything, so no pt_dataset is given
        with user_set(app, FakeUserAuth, 'user_without_any_coverage'):
            with requests_mock.Mocker() as m:
                m.get(requests_mock.ANY, json=response)
                r, status = self.query_no_assert('/v1/places?q=bob')
                assert status == 403

        # tgv has not access to the open_data but can use main_routing_test, it cannot use the global place
        with user_set(app, FakeUserAuth, 'tgv'):
            with requests_mock.Mocker() as m:
                _, status = self.query_no_assert('/v1/places?q=bob')
                assert status == 403
            # but it can use the main_routing_test places
            with requests_mock.Mocker() as m:
                _, status = self.query_no_assert('/v1/coverage/main_routing_test/places?q=bob')
                assert status == 200

        # super_user can use all the instances
        with user_set(app, FakeUserAuth, 'super_user'):
            with requests_mock.Mocker() as m:
                m.get(requests_mock.ANY, json=response)
                _, status = self.query_no_assert('/v1/places?q=bob')
                assert status == 200
                assert m.called
                check_dataset(m, {'main_routing_test', 'empty_routing_test', 'departure_board_test'})

        # but when querying /v1/coverage/<something>/places only one pt_dataset is given to bragi
        with user_set(app, FakeUserAuth, 'super_user'):
            with requests_mock.Mocker() as m:
                m.post(requests_mock.ANY, json=response)
                _, status = self.query_no_assert(
                    '/v1/coverage/main_routing_test/places?q=bob&_autocomplete=bragi'
                )
                assert status == 200
                assert m.called
                check_dataset(m, {'main_routing_test'})
            with requests_mock.Mocker() as m:
                m.get(requests_mock.ANY, json=response)
                _, status = self.query_no_assert(
                    '/v1/coverage/departure_board_test/places?q=bob&_autocomplete=bragi'
                )
                assert status == 200
                assert m.called
                check_dataset(m, {'departure_board_test'})

    def test_places_authentication_no_user(self):
        """a user is mandatory to use the places api API"""
        with requests_mock.Mocker() as m:
            _, status = self.query_no_assert('/v1/places?q=bob')
            assert status == 401
            _, status = self.query_no_assert('/v1/coverage/departure_board_test/places?q=bob')
            assert status == 401


@dataset(
    {"main_routing_test": {}, "departure_board_test": {}, "empty_routing_test": {}},
    global_config={'activate_bragi': True},
)
class AuthenticationPublicNavitia(AbstractTestFixture):
    def test_global_places_authentication(self):
        """
        On a public navitia, a user can use all the instances
        """
        with user_set(app, FakeUserAuth, 'bob'):
            with requests_mock.Mocker() as m:
                m.get(requests_mock.ANY, json=BRAGI_RESPONSE)
                r, status = self.query_no_assert('/v1/places?q=bob')
                assert status == 200
                assert m.called
                check_dataset(m, {'main_routing_test', 'empty_routing_test', 'departure_board_test'})

    def test_speficic_places_authentication(self):
        """
        for a specific coverage's places, there is still only one coverage
        """
        with user_set(app, FakeUserAuth, 'bob'):
            with requests_mock.Mocker() as m:
                m.post(requests_mock.ANY, json=BRAGI_RESPONSE)
                _, status = self.query_no_assert(
                    '/v1/coverage/main_routing_test/places?q=bob&_autocomplete=bragi'
                )
                assert status == 200
                assert m.called
                check_dataset(m, {'main_routing_test'})

    def test_global_places_authentication_no_user(self):
        """even without a user we can access all the places apis"""
        with requests_mock.Mocker() as m:
            m.post(requests_mock.ANY, json=BRAGI_RESPONSE)
            _, status = self.query_no_assert('/v1/coverage/main_routing_test/places?q=bob&_autocomplete=bragi')
            assert status == 200
            assert m.called
            check_dataset(m, {'main_routing_test'})

        with requests_mock.Mocker() as m:
            m.get(requests_mock.ANY, json=BRAGI_RESPONSE)
            _, status = self.query_no_assert('/v1/places?q=bob&_autocomplete=bragi')
            assert status == 200
            assert m.called
            check_dataset(m, {'main_routing_test', 'empty_routing_test', 'departure_board_test'})

    # TODO add more tests on:
    # * disruptions
    # * get by external code ?
