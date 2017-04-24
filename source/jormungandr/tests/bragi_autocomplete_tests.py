# coding=utf-8
# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
import mock
from jormungandr.tests.utils_test import MockRequests, MockResponse, user_set, FakeUser
from tests.check_utils import is_valid_global_autocomplete
from tests import check_utils
from tests.tests_mechanism import NewDefaultScenarioAbstractTestFixture
from .tests_mechanism import AbstractTestFixture, dataset
from nose.tools import raises
from jormungandr import app
from urllib import urlencode
from nose.tools import eq_
from .tests_mechanism import config


class FakeUserBragi(FakeUser):
    @classmethod
    def get_from_token(cls, token):
        """
        Create an empty user
        """
        return user_in_db_bragi[token]

def geojson():
    return '{"type": "Feature", "geometry": ' \
           '{"type": "Point", "coordinates": [102.0, 0.5]}, "properties": {"prop0": "value0"}}'

user_in_db_bragi = {
    'test_user_no_shape': FakeUserBragi('test_user_no_shape', 1,
                                        have_access_to_free_instances=False, is_super_user=True),
    'test_user_with_shape': FakeUserBragi('test_user_with_shape', 2,
                                          True, False, False, shape=geojson()),
}


MOCKED_INSTANCE_CONF = {
    'instance_config': {
        'default_autocomplete': 'bragi'
    }
}

BRAGI_MOCK_RESPONSE = {
    "features": [
        {
            "geometry": {
                "coordinates": [
                    3.282103,
                    49.847586
                ],
                "type": "Point"
            },
            "properties": {
                "geocoding": {
                    "city": "Bobtown",
                    "housenumber": "20",
                    "id": "49.847586;3.282103",
                    "label": "20 Rue Bob (Bobtown)",
                    "name": "Rue Bob",
                    "postcode": "02100",
                    "street": "Rue Bob",
                    "type": "house",
                    "citycode": "02000",
                    "administrative_regions": [
                        {
                            "id": "admin:fr:02000",
                            "insee": "02000",
                            "level": 8,
                            "label": "Bobtown (02000)",
                            "zip_codes": ["02000"],
                            "weight": 1,
                            "coord": {
                                "lat": 48.8396154,
                                "lon": 2.3957517
                            }
                        }
                    ],
                }
            },
            "type": "Feature"
        }
    ]
}

@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class TestBragiAutocomplete(AbstractTestFixture):

    def test_autocomplete_call(self):
        url = 'https://host_of_bragi/autocomplete'
        kwargs = {
            'params': {
                u'q': u'bob',
                u'type[]': [u'public_transport:stop_area', u'street', u'house', u'poi', u'city'],
                u'limit': 10,
                u'pt_dataset': 'main_routing_test',
            },
            'timeout': 10
        }

        url += "?{}".format(urlencode(kwargs.get('params'), doseq=True))
        mock_requests = MockRequests({
            url: (BRAGI_MOCK_RESPONSE, 200)
        })
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region")

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == '20 Rue Bob (Bobtown)'
            assert r[0]['embedded_type'] == 'address'
            assert r[0]['address']['name'] == 'Rue Bob'
            assert r[0]['address']['label'] == '20 Rue Bob (Bobtown)'

    def test_autocomplete_call_with_param_from(self):
        """
        test that the from param of the autocomplete is correctly given to bragi
        :return:
        """
        def http_get(url, *args, **kwargs):
            params = kwargs.pop('params')
            assert params
            assert params.get('lon') == '3.25'
            assert params.get('lat') == '49.84'
            return MockResponse({}, 200, '')
        with mock.patch('requests.get', http_get) as mock_method:
            self.query_region('places?q=bob&from=3.25;49.84')

    def test_autocomplete_call_override(self):
        """"
        test that the _autocomplete param switch the right autocomplete service
        """
        url = 'https://host_of_bragi'
        kwargs = {
            'params': {
                u'q': u'bob',
                u'type[]': [u'public_transport:stop_area', u'street', u'house', u'poi', u'city'],
                u'limit': 10,
                u'pt_dataset': 'main_routing_test',
            },
            'timeout': 10
        }

        url += "/autocomplete?{}".format(urlencode(kwargs.get('params'), doseq=True))

        mock_requests = MockRequests({
            url: (BRAGI_MOCK_RESPONSE, 200)
        })
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&type[]=stop_area&type[]=address&type[]=poi"
                                         "&type[]=administrative_region")

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == '20 Rue Bob (Bobtown)'
            assert r[0]['embedded_type'] == 'address'
            assert r[0]['address']['name'] == 'Rue Bob'
            assert r[0]['address']['label'] == '20 Rue Bob (Bobtown)'

            # with a query on kraken, the results should be different
            response = self.query_region("places?q=Park&_autocomplete=kraken")
            r = response.get('places')
            assert len(r) >= 1
            assert r[0]['name'] == 'first parking (Condom)'

    def test_autocomplete_call_with_no_param_type(self):
        """
        test that stop_area, poi, address and city are the default types passed to bragi
        :return:
        """
        def http_get(url, *args, **kwargs):
            params = kwargs.pop('params')
            assert params
            assert params.get('type[]') == ['public_transport:stop_area', 'street', 'house', 'poi', 'city']
            return MockResponse({}, 200, '')
        with mock.patch('requests.get', http_get) as mock_method:
            self.query_region('places?q=bob')

    def test_autocomplete_call_with_param_type_administrative_region(self):
        """
        test that administrative_region is converted to city
        :return:
        """
        def http_get(url, *args, **kwargs):
            params = kwargs.pop('params')
            assert params
            assert params.get('type[]') == ['city', 'street', 'house']

            return MockResponse({}, 200, '')
        with mock.patch('requests.get', http_get) as mock_method:
            self.query_region('places?q=bob&type[]=administrative_region&type[]=address')

    @raises(Exception)
    def test_autocomplete_call_with_param_type_not_acceptable(self):
        """
        test not acceptable type
        :return:
        """
        def http_get(url, *args, **kwargs):
            return MockResponse({}, 422, '')
        with mock.patch('requests.get', http_get) as mock_method:
            self.query_region('places?q=bob&type[]=bobette')

    def test_autocomplete_call_with_param_type_stop_point(self):
        """
        test that stop_point is not passed to bragi
        :return:
        """
        def http_get(url, *args, **kwargs):
            params = kwargs.pop('params')
            assert params
            assert params.get('type[]') == ['street', 'house']

            return MockResponse({}, 200, '')
        with mock.patch('requests.get', http_get) as mock_method:
            self.query_region('places?q=bob&type[]=stop_point&type[]=address')

    def test_features_call(self):
        url = 'https://host_of_bragi'
        kwargs = {
            'params': {
                u'pt_dataset': 'main_routing_test',
            },
            'timeout': 10
        }

        url += "/features/1234?{}".format(urlencode(kwargs.get('params'), doseq=True))

        mock_requests = MockRequests({
            url: (BRAGI_MOCK_RESPONSE, 200)
        })
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places/1234?&pt_dataset=main_routing_test")

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == '20 Rue Bob (Bobtown)'
            assert r[0]['embedded_type'] == 'address'
            assert r[0]['address']['name'] == 'Rue Bob'
            assert r[0]['address']['label'] == '20 Rue Bob (Bobtown)'

    def test_features_unknown_uri(self):
        url = 'https://host_of_bragi'
        kwargs = {
            'params': {
                u'pt_dataset': 'main_routing_test',
            },
            'timeout': 10
        }

        url += "/features/AAA?{}".format(urlencode(kwargs.get('params'), doseq=True))
        mock_requests = MockRequests({
        url:
            (
                {
                    'short": "query error',
                    'long": "invalid query EsError("Unable to find object")'
                },
                404
            )
        })

        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places/AAA?&pt_dataset=main_routing_test", check=False)
            assert response[1] == 404
            assert response[0]["error"]["id"] == 'unknown_object'
            assert response[0]["error"]["message"] == "The object AAA doesn't exist"


@dataset({"main_routing_test": {}}, global_config={'activate_bragi': True})
class TestBragiShape(AbstractTestFixture):

    def test_places_for_user_with_shape(self):
        """
        Test that with a shape on user, it is correctly posted
        """
        with user_set(app, FakeUserBragi, "test_user_with_shape"):

            mock_post = mock.MagicMock(return_value=MockResponse({}, 200, '{}'))

            def http_get(url, *args, **kwargs):
                assert False

            with mock.patch('requests.get', http_get):
                with mock.patch('requests.post', mock_post):

                    self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi')
                    assert mock_post.called

                    mock_post.reset_mock()
                    self.query('v1/places?q=toto')
                    assert mock_post.called

            # test that the shape is posted
            def http_post(url, *args, **kwargs):
                json = kwargs.pop('json')
                assert json['shape']['type'] == 'Feature'
                assert json.get('shape').get('geometry')
                return MockResponse({}, 200, '{}')

            with mock.patch('requests.get', http_get):
                with mock.patch('requests.post', http_post):
                    self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi')
                    self.query('v1/places?q=toto')

    def test_places_for_user_without_shape(self):
        """
        Test that without shape for user, we use the get method
        """
        with user_set(app, FakeUserBragi, "test_user_no_shape"):

            mock_get = mock.MagicMock(return_value=MockResponse({}, 200, '{}'))

            def http_post(self, url, *args, **kwargs):
                assert False

            with mock.patch('requests.get', mock_get):
                with mock.patch('requests.post', http_post):

                    self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi')
                    assert mock_get.called

                    mock_get.reset_mock()
                    self.query('v1/places?q=toto')
                    assert mock_get.called

    def test_global_place_uri(self):
        mock_requests = MockRequests({
            'https://host_of_bragi/features/bob': (BRAGI_MOCK_RESPONSE, 200)
        })
        with mock.patch('requests.get', mock_requests.get):
            response = self.query("/v1/places/bob")

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == '20 Rue Bob (Bobtown)'
            assert r[0]['embedded_type'] == 'address'
            assert r[0]['address']['name'] == 'Rue Bob'
            assert r[0]['address']['label'] == '20 Rue Bob (Bobtown)'



@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class AbstractAutocompleteAndRouting(AbstractTestFixture):
    def test_journey_with_external_uri_from_bragi(self):
        """
        This test aim to recreate a classic integration

        The user query the instance's autocomplete (which is set up to bragi)
        And then use the autocomplete's response to query for a journey

        For this test we have 2 item in our autocomplete:
         - the poi 'bobette' 
         - an adresse in bob's street that is not in the dataset
        """

        bragi_bobette = {
            "features": [
                {
                    "geometry": {
                        "coordinates": [
                            0.0000898312,
                            0.0000898312
                        ],
                        "type": "Point"
                    },
                    "properties": {
                        "geocoding": {
                            "city": "Bobtown",
                            "id": "bobette",
                            "label": "bobette's label",
                            "name": "bobette",
                            "poi_types": [
                                {
                                    "id": "poi_type:amenity:bicycle_rental", 
                                    "name": "Station VLS"
                                }
                            ], 
                            "postcode": "02100",
                            "type": "poi",
                            "citycode": "02000",
                            "administrative_regions": [
                                {
                                    "id": "admin:fr:02000",
                                    "insee": "02000",
                                    "level": 8,
                                    "label": "Bobtown (02000)",
                                    "zip_codes": ["02000"],
                                    "weight": 1,
                                    "coord": {
                                        "lat": 48.8396154,
                                        "lon": 2.3957517
                                    }
                                }
                            ],
                        }
                    },
                    "type": "Feature"
                }
            ]
        }

        bob_street = {
            "features": [
                {
                    "geometry": {
                        "coordinates": [
                            0.00188646,
                            0.00071865
                        ],
                        "type": "Point"
                    },
                    "properties": {
                        "geocoding": {
                            "city": "Bobtown",
                            "housenumber": "20",
                            "id": "addr:" + check_utils.r_coord, # the adresse is just above 'R'
                            "label": "20 Rue Bob (Bobtown)",
                            "name": "Rue Bob",
                            "postcode": "02100",
                            "street": "Rue Bob",
                            "type": "house",
                            "citycode": "02000",
                            "administrative_regions": [
                                {
                                    "id": "admin:fr:02000",
                                    "insee": "02000",
                                    "level": 8,
                                    "label": "Bobtown (02000)",
                                    "zip_codes": ["02000"],
                                    "weight": 1,
                                    "coord": {
                                        "lat": 48.8396154,
                                        "lon": 2.3957517
                                    }
                                }
                            ],
                        }
                    },
                    "type": "Feature"
                }
            ]
        }

        args = {
            u'pt_dataset': 'main_routing_test',
            u'type[]': [u'public_transport:stop_area', u'street', u'house', u'poi', u'city'],
            u'limit': 10,
        }
        params = urlencode(args, doseq=True)
        
        mock_requests = MockRequests({
            'https://host_of_bragi/autocomplete?q=bobette&{p}'.format(p=params): (bragi_bobette, 200),
            'https://host_of_bragi/features/bobette?pt_dataset=main_routing_test': (bragi_bobette, 200),
            'https://host_of_bragi/autocomplete?q=20+rue+bob&{p}'.format(p=params): (bob_street, 200),
            'https://host_of_bragi/reverse?lat={lat}&lon={lon}&pt_dataset=main_routing_test'
            .format(lon=check_utils.r_coord.split(';')[0], lat=check_utils.r_coord.split(';')[1])
            : (bob_street, 200)
        })

        def get_autocomplete(query):
            autocomplete_response = self.query_region(query)
            r = autocomplete_response.get('places')
            assert len(r) == 1
            return r[0]['id']

        with mock.patch('requests.get', mock_requests.get):
            journeys_from = get_autocomplete('places?q=bobette')
            journeys_to = get_autocomplete('places?q=20 rue bob')
            query = 'journeys?from={f}&to={to}&datetime={dt}'.format(f=journeys_from, to=journeys_to, dt="20120614T080000")
            journeys_response = self.query_region(query)

            self.is_valid_journey_response(journeys_response, query)

            # all journeys should have kept the user's from/to
            for j in journeys_response['journeys']:
                response_from = j['sections'][0]['from']
                eq_(response_from['id'], "bobette")
                eq_(response_from['name'], "bobette's label")
                eq_(response_from['embedded_type'], "poi")
                eq_(response_from['poi']['label'], "bobette's label")

                response_to = j['sections'][-1]['to']
                eq_(response_to['id'], journeys_to)
                eq_(response_to['name'], "20 Rue Bob (Bobtown)")
                eq_(response_to['embedded_type'], "address")
                eq_(response_to['address']['label'], "20 Rue Bob (Bobtown)")

@config({'scenario': 'new_default'})
class TestNewDefaultAutocompleteAndRouting(AbstractAutocompleteAndRouting,
                                         NewDefaultScenarioAbstractTestFixture):
    pass

@config({'scenario': 'experimental'})
class TestExperimentalAutocompleteAndRouting(AbstractAutocompleteAndRouting,
                                         NewDefaultScenarioAbstractTestFixture):
    pass
