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
from pytest import raises

from jormungandr.tests.utils_test import MockRequests, MockResponse, user_set, FakeUser
from tests.check_utils import is_valid_global_autocomplete
from tests import check_utils
from tests.tests_mechanism import NewDefaultScenarioAbstractTestFixture
from .tests_mechanism import AbstractTestFixture, dataset
from jormungandr import app
from six.moves.urllib.parse import urlencode
from .tests_mechanism import config
from copy import deepcopy
import os

class FakeUserBragi(FakeUser):
    @classmethod
    def get_from_token(cls, token, valid_until):
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
    'test_user_with_coord': FakeUserBragi('test_user_with_coord', 2,
                                          True, False, False, default_coord="12;42"),
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
                            "name": "Bobtown",
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


BRAGI_MOCK_TYPE_UNKNOWN = {
    "type": "FeatureCollection",
    "geocoding": {
        "version": "0.1.0",
        "query": ""
    },
    "features": [
        {
            "type": "Feature",
            "geometry": {
                "coordinates": [
                    3.1092154,
                    50.6274528
                ],
                "type": "Point"
            },
            "properties": {
                "geocoding": {
                    "id": "admin:osm:2643160",
                    "type": "unknown",
                    "label": "Hellemmes-Lille",
                    "name": "Hellemmes-Lille",
                    "postcode": None,
                    "city": None,
                    "citycode": "",
                    "level": 9,
                    "administrative_regions": []
                }
            }
        },
        {
            "type": "Feature",
            "geometry": {
                "coordinates": [
                    3.0706414,
                    50.6305089
                ],
                "type": "Point"
            },
            "properties": {
                "geocoding": {
                    "id": "admin:fr:59350",
                    "type": "city",
                    "label": "Lille (59000-59800)",
                    "name": "Lille",
                    "postcode": "59000;59160;59260;59777;59800",
                    "city": None,
                    "citycode": "59350",
                    "level": 8,
                    "administrative_regions": []
                }
            }
        }
    ]
}


BRAGI_MOCK_POI_WITHOUT_ADDRESS = {
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
                    "properties": [
                        {"key": "amenity", "value": "bicycle_rental"},
                        {"key": "capacity", "value": "20"},
                        {"key": "ref", "value": "12"}
                    ],
                    "administrative_regions": [
                        {
                            "id": "admin:fr:02000",
                            "insee": "02000",
                            "level": 8,
                            "label": "Bobtown (02000)",
                            "name": "Bobtown",
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

BRAGI_MOCK_STOP_AREA_WITH_MORE_ATTRIBUTS = {
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
                    "type": "public_transport:stop_area",
                    "citycode": "02000",
                    "properties": [
                        {"key": "name",
                         "value": "railway station"},
                        {"key": "code",
                         "value": "station:01"}
                    ],
                    "commercial_modes": [
                        {"id": "cm_id:Bus",
                         "name": "cm_name:Bus"},
                        {"id": "cm_id:Car",
                         "name": "cm_name:Car"}
                    ],
                    "physical_modes": [
                        {"id": "pm_id:Bus",
                         "name": "pm_name:Bus"},
                        {"id": "pm_id:Car",
                         "name": "pm_name:Car"}
                    ],
                    "codes": [
                        {"name": "navitia1",
                         "value": "424242"},
                        {"name": "source",
                         "value": "1161"}
                    ],
                    "timezone": "Europe/Paris",
                    "administrative_regions": [
                        {
                            "id": "admin:fr:02000",
                            "insee": "02000",
                            "level": 8,
                            "label": "Bobtown (02000)",
                            "name": "Bobtown",
                            "zip_codes": ["02000"],
                            "weight": 1,
                            "coord": {
                                "lat": 48.8396154,
                                "lon": 2.3957517
                            }
                        }
                    ],
                    "feed_publishers": [
                        {
                            "id": "bob_publisher",
                            "license": "OBobDL",
                            "name": "Bobenstreetmap",
                            "url": "https://www.Bobenstreetmap.org/copyright"
                        }
                    ],
                    }
            },
            "type": "Feature"
        }
    ]
}

BRAGI_MOCK_STOP_AREA_WITH_BASIC_ATTRIBUTS = {
    "features": [
        {
            "geometry": {
                "coordinates": [
                    0.0000898312,#has to match with kraken mock
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
                    "type": "public_transport:stop_area",
                    "citycode": "02000",
                    "administrative_regions": [
                        {
                            "id": "admin:fr:02000",
                            "insee": "02000",
                            "level": 8,
                            "label": "Bobtown (02000)",
                            "name": "Bobtown",
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

BRAGI_MOCK_BOBETTE = {
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
                    "properties": [
                        {"key": "amenity", "value": "bicycle_rental"},
                        {"key": "capacity", "value": "20"},
                        {"key": "ref", "value": "12"}
                    ],
                    "address": {
                        "type": "street",
                        "id": "addr:8.9809147;42.561667",
                        "name": "Speloncato-Monticello",
                        "label": "Speloncato-Monticello (Speloncato)",
                        "postcode": "20226",
                        "city": "Speloncato",
                        "citycode": "2B290",
                        "administrative_regions": [
                            {
                                "id": "admin:fr:02000",
                                "insee": "02000",
                                "level": 8,
                                "label": "Bobtown (02000)",
                                "name": "Bobtown",
                                "zip_codes": ["02000"],
                                "weight": 1,
                                "coord": {
                                    "lat": 48.8396154,
                                    "lon": 2.3957517
                                }
                            }
                        ],
                        "weight": 0.00847457627118644
                    },
                    "administrative_regions": [
                        {
                            "id": "admin:fr:02000",
                            "insee": "02000",
                            "level": 8,
                            "label": "Bobtown (02000)",
                            "name": "Bobtown",
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
#Original coordinates are used to calculate id. We need different lat and lon for test
BRAGI_MOCK_BOBETTE_DEPTH_ZERO = deepcopy(BRAGI_MOCK_BOBETTE)
BRAGI_MOCK_BOBETTE_DEPTH_ZERO["features"][0]["geometry"] = \
    {
        "coordinates": [
            2.3957517,
            48.8396154
        ],
        "type": "Point"
    }
BRAGI_MOCK_BOBETTE_DEPTH_ONE = deepcopy(BRAGI_MOCK_BOBETTE_DEPTH_ZERO)
BRAGI_MOCK_BOBETTE_DEPTH_TWO = deepcopy(BRAGI_MOCK_BOBETTE_DEPTH_ZERO)
BRAGI_MOCK_BOBETTE_DEPTH_THREE = deepcopy(BRAGI_MOCK_BOBETTE_DEPTH_ZERO)

BOB_STREET = {
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
                            "name": "Bobtown",
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

BRAGI_MOCK_ADMIN = {
    "type": "FeatureCollection",
    "geocoding": {
        "version": "0.1.0",
        "query": ""
    },
    "features": [
        {
            "type": "Feature",
            "geometry": {
                "coordinates": [
                    5.0414701,
                    47.3215806
                ],
                "type": "Point"
            },
            "properties": {
                "geocoding": {
                    "id": "admin:fr:21231",
                    "type": "city",
                    "label": "Dijon (21000)",
                    "name": "Dijon",
                    "postcode": "21000",
                    "city": None,
                    "citycode": "21231",
                    "level": 8,
                    "administrative_regions": []
                }
            }
        }
    ]
}

BRAGI_MOCK_ADMINISTRATIVE_REGION = {
    "type": "FeatureCollection",
    "geocoding": {
        "version": "0.1.0",
        "query": ""
    },
    "features": [
        {
            "type": "Feature",
            "geometry": {
                "coordinates": [
                    2.4518371,
                    48.7830727
                ],
                "type": "Point"
            },
            "properties": {
                "geocoding": {
                    "id": "admin:fr:941",
                    "type": "administrative_region",
                    "label": "Créteil (94000)",
                    "name": "Créteil",
                    "postcode": "94000",
                    "city": None,
                    "citycode": None,
                    "level": 7,
                    "administrative_regions": []
                }
            }
        }
    ]
}

BRAGI_MOCK_ADMINISTRATIVE_REGION_WITH_WRONG_TYPE = {
    "type": "FeatureCollection",
    "geocoding": {
        "version": "0.1.0",
        "query": ""
    },
    "features": [
        {
            "type": "Feature",
            "geometry": {
                "coordinates": [
                    2.4518371,
                    48.7830727
                ],
                "type": "Point"
            },
            "properties": {
                "geocoding": {
                    "id": "myId",
                    "type": "THIS_IS_NOT_A_WORKING_TYPE",
                    "label": "myLabel",
                    "name": "myName",
                    "postcode": "123456",
                    "city": None,
                    "citycode": None,
                    "level": 7,
                    "administrative_regions": []
                }
            }
        }
    ]
}

BRAGI_MOCK_RESPONSE_STOP_AREA_WITH_COMMENTS = {
    "type": "FeatureCollection",
    "geocoding": {
        "version": "0.1.0",
        "query": ""
    },
    "features": [
        {
            "type": "Feature",
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
                    "id": "stop_area_id",
                    "label": "stop 1",
                    "name": "stop 1",
                    "postcode": "02100",
                    "type": "public_transport:stop_area",
                    "citycode": "02000",
                    "administrative_regions": [],
                    "commercial_modes": [
                        {
                            "id": "commercial_mode:Bus",
                            "name": "Bus"
                        },
                    ],
                    "physical_modes": [
                        {
                            "id": "commercial_mode:Bus",
                            "name": "Bus"
                        },
                    ],
                    "comments": [
                        {
                            "name": "comment1",
                        },
                        {
                            "name": "comment2",
                        }
                    ],
                    "timezone": "Europe/Paris",
                    "codes": [
                        {
                            "name": "code_name1",
                            "value": "1"
                        },
                        {
                            "name": "coce_name2",
                            "value": "2"
                        }
                    ],
                    "feed_publishers": [
                        {
                            "id": "feed_p_id",
                            "license": "feed_p_license",
                            "name": "feed_p_name",
                            "url": "feed_p_url"
                        }
                    ]
                }
            },
        }
    ]
}

BRAGI_MOCK_RESPONSE_STOP_AREA_WITHOUT_COMMENTS = {
    "type": "FeatureCollection",
    "geocoding": {
        "version": "0.1.0",
        "query": ""
    },
    "features": [
        {
            "type": "Feature",
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
                    "id": "stop_area_id",
                    "label": "stop 1",
                    "name": "stop 1",
                    "postcode": "02100",
                    "type": "public_transport:stop_area",
                    "citycode": "02000",
                    "administrative_regions": [],
                    "commercial_modes": [
                        {
                            "id": "commercial_mode:Bus",
                            "name": "Bus"
                        },
                    ],
                    "physical_modes": [
                        {
                            "id": "commercial_mode:Bus",
                            "name": "Bus"
                        },
                    ],
                    "timezone": "Europe/Paris",
                    "codes": [
                        {
                            "name": "code_name1",
                            "value": "1"
                        },
                        {
                            "name": "coce_name2",
                            "value": "2"
                        }
                    ],
                    "feed_publishers": [
                        {
                            "id": "feed_p_id",
                            "license": "feed_p_license",
                            "name": "feed_p_name",
                            "url": "feed_p_url"
                        }
                    ]
                }
            },
        }
    ]
}

def mock_bragi_autocomplete_call(bragi_response, limite=10, http_response_code=200):
    url = 'https://host_of_bragi/autocomplete'
    params = {
        'q': u'bob',
        'type[]': [u'public_transport:stop_area', u'street', u'house', u'poi', u'city'],
        'limit': limite,
        'pt_dataset': 'main_routing_test'
    }

    url += "?{}".format(urlencode(params, doseq=True))
    mock_requests = MockRequests({
        url: (bragi_response, http_response_code)
    })

    return mock_requests

@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class TestBragiAutocomplete(AbstractTestFixture):

    def test_autocomplete_call(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_RESPONSE)
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
            fbs = response['feed_publishers']
            assert {fb['id'] for fb in fbs} >= {u'osm', u'bano'}
            assert len(r[0]['address'].get('administrative_regions')) == 1

    def test_autocomplete_call_depth_zero(self):
        mock_requests = mock_bragi_autocomplete_call(deepcopy(BRAGI_MOCK_RESPONSE))
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region&depth=0")

            is_valid_global_autocomplete(response, depth=0)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == '20 Rue Bob (Bobtown)'
            assert r[0]['embedded_type'] == 'address'
            assert r[0]['address']['name'] == 'Rue Bob'
            assert r[0]['address']['label'] == '20 Rue Bob (Bobtown)'
            fbs = response['feed_publishers']
            assert {fb['id'] for fb in fbs} >= {u'osm', u'bano'}
            assert 'administrative_regions' not in r[0]['address']

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
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_RESPONSE)
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

    def test_autocomplete_call_with_param_type_not_acceptable(self):
        """
        test not acceptable type
        :return:
        """
        def http_get(url, *args, **kwargs):
            return MockResponse({}, 422, '')

        with raises(Exception):
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
        params = {'pt_dataset': 'main_routing_test'}

        url += "/features/1234?{}".format(urlencode(params, doseq=True))

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
        params = {'pt_dataset': 'main_routing_test'}

        url += "/features/AAA?{}".format(urlencode(params, doseq=True))
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

    def test_poi_without_address(self):
        url = 'https://host_of_bragi'
        params = {'pt_dataset': 'main_routing_test'}

        url += "/features/1234?{}".format(urlencode(params, doseq=True))

        mock_requests = MockRequests({
            url: (BRAGI_MOCK_POI_WITHOUT_ADDRESS, 200)
        })
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places/1234?&pt_dataset=main_routing_test")

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['embedded_type'] == 'poi'
            assert r[0]['poi']['name'] == 'bobette'
            assert r[0]['poi']['label'] == "bobette's label"
            assert not r[0]['poi'].get('address')

    def test_stop_area_with_modes(self):
        url = 'https://host_of_bragi'
        params = {'pt_dataset': 'main_routing_test'}

        url += "/features/1234?{}".format(urlencode(params, doseq=True))

        mock_requests = MockRequests({
            url: (BRAGI_MOCK_STOP_AREA_WITH_MORE_ATTRIBUTS, 200)
        })
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places/1234?&pt_dataset=main_routing_test")

            assert response.get('feed_publishers')
            if os.getenv('JORMUNGANDR_USE_SERPY'):
                assert len(response.get('feed_publishers')) == 3
            else:
                assert len(response.get('feed_publishers')) == 2

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['embedded_type'] == 'stop_area'
            assert r[0]['stop_area']['name'] == 'bobette'
            assert len(r[0]['stop_area'].get('commercial_modes')) == 2
            assert r[0]['stop_area'].get('commercial_modes')[0].get('id') == 'cm_id:Bus'
            assert r[0]['stop_area'].get('commercial_modes')[0].get('name') == 'cm_name:Bus'
            assert r[0]['stop_area'].get('commercial_modes')[1].get('id') == 'cm_id:Car'
            assert r[0]['stop_area'].get('commercial_modes')[1].get('name') == 'cm_name:Car'

            assert len(r[0]['stop_area'].get('physical_modes')) == 2
            assert r[0]['stop_area'].get('physical_modes')[0].get('id') == 'pm_id:Bus'
            assert r[0]['stop_area'].get('physical_modes')[0].get('name') == 'pm_name:Bus'
            assert r[0]['stop_area'].get('physical_modes')[1].get('id') == 'pm_id:Car'
            assert r[0]['stop_area'].get('physical_modes')[1].get('name') == 'pm_name:Car'

            assert len(r[0]['stop_area'].get('codes')) == 2
            assert r[0]['stop_area'].get('codes')[0].get('type') == 'external_code'
            assert r[0]['stop_area'].get('codes')[0].get('value') == '424242'
            assert r[0]['stop_area'].get('codes')[1].get('type') == 'source'
            assert r[0]['stop_area'].get('codes')[1].get('value') == '1161'

            assert r[0]['stop_area'].get('properties').get('name') == 'railway station'
            assert r[0]['stop_area'].get('properties').get('code') == 'station:01'

            assert r[0]['stop_area'].get('timezone') == 'Europe/Paris'
            admins = r[0]['stop_area'].get('administrative_regions')
            assert len(admins) == 1

    def test_stop_area_with_modes_depth_zero(self):
        mock_requests = mock_bragi_autocomplete_call(deepcopy(BRAGI_MOCK_STOP_AREA_WITH_MORE_ATTRIBUTS))
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region&depth=0")

            assert response.get('feed_publishers')
            if os.getenv('JORMUNGANDR_USE_SERPY'):
                assert len(response.get('feed_publishers')) == 3
            else:
                assert len(response.get('feed_publishers')) == 2

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['embedded_type'] == 'stop_area'
            assert r[0]['stop_area']['name'] == 'bobette'
            assert len(r[0]['stop_area'].get('commercial_modes')) == 2
            assert r[0]['stop_area'].get('commercial_modes')[0].get('id') == 'cm_id:Bus'
            assert r[0]['stop_area'].get('commercial_modes')[0].get('name') == 'cm_name:Bus'
            assert r[0]['stop_area'].get('commercial_modes')[1].get('id') == 'cm_id:Car'
            assert r[0]['stop_area'].get('commercial_modes')[1].get('name') == 'cm_name:Car'

            assert len(r[0]['stop_area'].get('physical_modes')) == 2
            assert r[0]['stop_area'].get('physical_modes')[0].get('id') == 'pm_id:Bus'
            assert r[0]['stop_area'].get('physical_modes')[0].get('name') == 'pm_name:Bus'
            assert r[0]['stop_area'].get('physical_modes')[1].get('id') == 'pm_id:Car'
            assert r[0]['stop_area'].get('physical_modes')[1].get('name') == 'pm_name:Car'

            assert len(r[0]['stop_area'].get('codes')) == 2
            assert r[0]['stop_area'].get('codes')[0].get('type') == 'external_code'
            assert r[0]['stop_area'].get('codes')[0].get('value') == '424242'
            assert r[0]['stop_area'].get('codes')[1].get('type') == 'source'
            assert r[0]['stop_area'].get('codes')[1].get('value') == '1161'

            assert r[0]['stop_area'].get('properties').get('name') == 'railway station'
            assert r[0]['stop_area'].get('properties').get('code') == 'station:01'

            assert r[0]['stop_area'].get('timezone') == 'Europe/Paris'
            # With depth = 0 no administrative_regions
            assert 'administrative_regions' not in r[0]['stop_area']

    def test_stop_area_without_modes(self):
        url = 'https://host_of_bragi'
        params = {'pt_dataset': 'main_routing_test'}

        url += "/features/1234?{}".format(urlencode(params, doseq=True))

        mock_requests = MockRequests({
            url: (BRAGI_MOCK_STOP_AREA_WITH_BASIC_ATTRIBUTS, 200)
        })
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places/1234?&pt_dataset=main_routing_test")

            assert response.get('feed_publishers')
            assert len(response.get('feed_publishers')) == 2

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['embedded_type'] == 'stop_area'
            assert 'commercial_modes' not in r[0]['stop_area']
            assert 'physical_modes' not in r[0]['stop_area']
            # Empty attribute not displayed
            assert 'codes' not in r[0]['stop_area']
            # Attribute empty not displayed
            assert 'properties' not in r[0]['stop_area']
            # Attribute displayed but None
            assert not r[0]['stop_area'].get('timezone')

    def test_feature_unknown_type(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_TYPE_UNKNOWN, limite=2)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query("v1/places?q=bob&count=2")

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            #check that we get only one response, the other one being filtered as type is unknown
            assert len(r) == 1
            assert r[0]['name'] == 'Lille'
            assert r[0]['embedded_type'] == 'administrative_region'
            assert r[0]['id']== 'admin:fr:59350'
            assert r[0]['administrative_region']['label'] == 'Lille (59000-59800)'

    def test_autocomplete_call_with_depth_zero(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_BOBETTE_DEPTH_ZERO)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region&depth=0")

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == "bobette's label"
            assert r[0]['embedded_type'] == "poi"
            poi = r[0]['poi']
            assert poi['label'] == "bobette's label"
            assert poi['coord']['lat'] == '48.8396154'
            assert poi['coord']['lon'] == '2.3957517'
            assert poi['properties']["amenity"] == "bicycle_rental"
            assert poi['properties']["capacity"] == "20"
            assert poi['properties']["ref"] == "12"
            #Empty administrative_regions not displayed as in kraken
            assert not poi.get('administrative_regions')
            assert 'administrative_regions' not in poi
            #Address absent as in kraken
            assert 'address' not in poi

    def test_autocomplete_call_with_depth_one(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_BOBETTE_DEPTH_ONE)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region&depth=1")

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == "bobette's label"
            assert r[0]['embedded_type'] == "poi"
            poi = r[0]['poi']
            assert poi['label'] == "bobette's label"
            assert poi['properties']["amenity"] == "bicycle_rental"
            assert poi['properties']["capacity"] == "20"
            assert poi['properties']["ref"] == "12"
            poi_admins = poi['administrative_regions']
            assert len(poi_admins) == 1
            assert poi_admins[0]['id'] == "admin:fr:02000"
            assert poi_admins[0]['insee'] == "02000"
            assert poi_admins[0]['label'] == "Bobtown (02000)"
            assert poi_admins[0]['name'] == "Bobtown"
            assert poi_admins[0]['coord']['lat'] == "48.8396154"
            assert poi_admins[0]['coord']['lon'] == "2.3957517"

            address = poi['address']
            assert address['coord']['lat'] == '48.8396154'
            assert address['coord']['lon'] == '2.3957517'
            assert address['id'] == "2.3957517;48.8396154"
            assert address['name'] == "Speloncato-Monticello"
            assert address['house_number'] == 0
            #Empty administrative_regions not displayed as in kraken
            assert not address.get('administrative_regions')

    def test_autocomplete_call_with_depth_two(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_BOBETTE_DEPTH_TWO)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region&depth=2")

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == "bobette's label"
            assert r[0]['embedded_type'] == "poi"
            poi = r[0]['poi']
            assert poi['label'] == "bobette's label"
            assert poi['properties']["amenity"] == "bicycle_rental"
            assert poi['properties']["capacity"] == "20"
            assert poi['properties']["ref"] == "12"
            poi_admins = poi['administrative_regions']
            assert len(poi_admins) == 1
            assert poi_admins[0]['id'] == "admin:fr:02000"
            assert poi_admins[0]['insee'] == "02000"
            assert poi_admins[0]['label'] == "Bobtown (02000)"
            assert poi_admins[0]['name'] == "Bobtown"
            assert poi_admins[0]['coord']['lat'] == "48.8396154"
            assert poi_admins[0]['coord']['lon'] == "2.3957517"

            address = poi['address']
            assert address['coord']['lat'] == '48.8396154'
            assert address['coord']['lon'] == '2.3957517'
            assert address['id'] == "2.3957517;48.8396154"
            assert address['name'] == "Speloncato-Monticello"
            assert address['house_number'] == 0

            address_admins = address['administrative_regions']
            assert len(address_admins) == 1
            assert address_admins[0]['id'] == "admin:fr:02000"
            assert address_admins[0]['insee'] == "02000"
            assert address_admins[0]['label'] == "Bobtown (02000)"
            assert address_admins[0]['name'] == "Bobtown"
            assert address_admins[0]['coord']['lat'] == "48.8396154"
            assert address_admins[0]['coord']['lon'] == "2.3957517"

    #This test is to verify that query with depth = 2 and 3 gives the same result as in kraken
    def test_autocomplete_call_with_depth_three(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_BOBETTE_DEPTH_THREE)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region&depth=3")

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == "bobette's label"
            assert r[0]['embedded_type'] == "poi"
            poi = r[0]['poi']
            assert poi['label'] == "bobette's label"
            assert poi['properties']["amenity"] == "bicycle_rental"
            assert poi['properties']["capacity"] == "20"
            assert poi['properties']["ref"] == "12"
            poi_admins = poi['administrative_regions']
            assert len(poi_admins) == 1
            assert poi_admins[0]['id'] == "admin:fr:02000"
            assert poi_admins[0]['insee'] == "02000"
            assert poi_admins[0]['label'] == "Bobtown (02000)"
            assert poi_admins[0]['name'] == "Bobtown"
            assert poi_admins[0]['coord']['lat'] == "48.8396154"
            assert poi_admins[0]['coord']['lon'] == "2.3957517"

            address = poi['address']
            assert address['coord']['lat'] == '48.8396154'
            assert address['coord']['lon'] == '2.3957517'
            assert address['id'] == "2.3957517;48.8396154"
            assert address['name'] == "Speloncato-Monticello"
            assert address['house_number'] == 0

            address_admins = address['administrative_regions']
            assert len(address_admins) == 1
            assert address_admins[0]['id'] == "admin:fr:02000"
            assert address_admins[0]['insee'] == "02000"
            assert address_admins[0]['label'] == "Bobtown (02000)"
            assert address_admins[0]['name'] == "Bobtown"
            assert address_admins[0]['coord']['lat'] == "48.8396154"
            assert address_admins[0]['coord']['lon'] == "2.3957517"

    def test_autocomplete_for_admin_depth_zero(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_ADMIN)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region&depth=0")

            is_valid_global_autocomplete(response, depth=0)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == 'Dijon'
            assert r[0]['embedded_type'] == 'administrative_region'
            assert r[0]['id'] == 'admin:fr:21231'
            assert r[0]['administrative_region']['id'] == 'admin:fr:21231'
            assert r[0]['administrative_region']['insee'] == '21231'
            assert r[0]['administrative_region']['label'] == 'Dijon (21000)'
            assert r[0]['administrative_region']['name'] == 'Dijon'
            assert 'administrative_regions' not in r[0]['administrative_region']

    def test_autocomplete_for_administrative_region(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_ADMINISTRATIVE_REGION)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob")
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == 'Créteil'
            assert r[0]['id'] == 'admin:fr:941'
            assert r[0]['embedded_type'] == 'administrative_region'
            assert r[0]['administrative_region']['name'] == 'Créteil'
            assert r[0]['administrative_region']['level'] == 7
            assert r[0]['administrative_region']['label'] == 'Créteil (94000)'
            assert r[0]['administrative_region']['id'] == 'admin:fr:941'
            assert r[0]['administrative_region']['zip_code'] == '94000'

    def test_autocomplete_for_administrative_region_with_wrong_type(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_ADMINISTRATIVE_REGION_WITH_WRONG_TYPE)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob")
            r = response.get('places')
            assert len(r) == 0
            
    # Since administrative_regions of the admin is an empty list in the result bragi
    # there is no difference in the final result with depth from 0 to 3
    def test_autocomplete_for_admin_depth_two(self):
        mock_requests = mock_bragi_autocomplete_call(deepcopy(BRAGI_MOCK_ADMIN))
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region&depth=2")

            is_valid_global_autocomplete(response, depth=2)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == 'Dijon'
            assert r[0]['embedded_type'] == 'administrative_region'
            assert r[0]['id'] == 'admin:fr:21231'
            assert r[0]['administrative_region']['id'] == 'admin:fr:21231'
            assert r[0]['administrative_region']['insee'] == '21231'
            assert r[0]['administrative_region']['label'] == 'Dijon (21000)'
            assert r[0]['administrative_region']['name'] == 'Dijon'
            # In our case administrative_regions of the admin is an empty list in the result bragi
            assert 'administrative_regions' not in r[0]['administrative_region']

    def test_autocomplete_call_with_comments_on_stop_area(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_RESPONSE_STOP_AREA_WITH_COMMENTS)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region")
            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == 'stop 1'
            assert r[0]['embedded_type'] == 'stop_area'
            assert r[0]['id'] == 'stop_area_id'
            assert r[0]['quality'] == 0 # field for kraken compatibility (default = 0)
            stop_area = r[0]['stop_area']
            # For retrocompatibility, when we have multi-comments, we keep 'comment' field with
            # the first comment element.
            assert stop_area['comment'] == 'comment1'
            comments = stop_area['comments']
            assert {comment['value'] for comment in comments} >= {u'comment1', u'comment2'}

    def test_autocomplete_call_without_comments_on_stop_area(self):
        mock_requests = mock_bragi_autocomplete_call(BRAGI_MOCK_RESPONSE_STOP_AREA_WITHOUT_COMMENTS)
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region("places?q=bob&pt_dataset=main_routing_test&type[]=stop_area"
                                         "&type[]=address&type[]=poi&type[]=administrative_region")
            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == 'stop 1'
            assert r[0]['embedded_type'] == 'stop_area'
            assert r[0]['id'] == 'stop_area_id'
            assert r[0]['quality'] == 0 # field for kraken compatibility (default = 0)
            stop_area = r[0]['stop_area']
            # When no comments exist in Bragi, the API mask the "comment" and "comments" field.
            assert not stop_area.get('comment')
            assert not stop_area.get('comments')


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

    def test_places_for_user_with_coord(self):
        """
        Test that with a default_coord on user, it is correctly posted
        """
        with user_set(app, FakeUserBragi, "test_user_with_coord"):
            def http_get(url, *args, **kwargs):
                params = kwargs.pop('params')
                assert params
                assert params.get('lon') == '12'
                assert params.get('lat') == '42'
                return MockResponse({}, 200, '')

            with mock.patch('requests.get', http_get):
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi')

    def test_places_for_user_with_coord_and_coord_overriden(self):
        """
        Test that with a default_coord on user, if the user gives a coord we use the given coord
        """
        with user_set(app, FakeUserBragi, "test_user_with_coord"):
            def http_get(url, *args, **kwargs):
                params = kwargs.pop('params')
                assert params
                assert params.get('lon') == '1'
                assert params.get('lat') == '2'
                return MockResponse({}, 200, '')

            with mock.patch('requests.get', http_get):
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi&from=1;2')

    def test_places_for_user_with_coord_and_coord_overriden_to_null(self):
        """
        Test that with a default_coord on user, if the user gives an empty coord we do not pass a coord
        """
        with user_set(app, FakeUserBragi, "test_user_with_coord"):
            def http_get(url, *args, **kwargs):
                params = kwargs.pop('params')
                assert params
                assert not params.get('lon')
                assert not params.get('lat')
                return MockResponse({}, 200, '')

            with mock.patch('requests.get', http_get):
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi&from=')

    def test_places_with_empty_coord(self):
        """
        Test that we get an error if we give an empty coord
        (and if there is no user defined coord to override)
        """
        r, s = self.query_no_assert('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi&from=')
        assert s == 400
        assert "if 'from' is provided it cannot be null" in r.get('message')

    def test_global_place_uri(self):
        mock_requests = MockRequests({
            # there is no authentication so all the known pt_dataset are added as parameters
            'https://host_of_bragi/features/bob?pt_dataset=main_routing_test': (BRAGI_MOCK_RESPONSE, 200)
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

    def test_global_coords_uri(self):
        url = 'https://host_of_bragi'
        params = {
            'pt_dataset': 'main_routing_test',
            'lon': 3.282103,
            'lat': 49.84758
        }
        url += "/reverse?{}".format(urlencode(params, doseq=True))

        mock_requests = MockRequests({
            url: (BRAGI_MOCK_RESPONSE, 200)
        })

        with mock.patch('requests.get', mock_requests.get):
            response = self.query("/v1/coverage/{pt_dataset}/coords/{lon};{lat}?_autocomplete=bragi".format(
                lon=params.get('lon'), lat=params.get('lat'), pt_dataset=params.get('pt_dataset')))

            address = response.get('address')
            assert address
            assert address['house_number'] == 20
            assert address['name'] == 'Rue Bob'
            assert address['label'] == '20 Rue Bob (Bobtown)'
            assert len(address['administrative_regions']) == 1


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class AbstractAutocompleteAndRouting():
    def test_journey_with_external_uri_from_bragi(self):
        """
        This test aim to recreate a classic integration

        The user query the instance's autocomplete (which is set up to bragi)
        And then use the autocomplete's response to query for a journey

        For this test we have 2 item in our autocomplete:
         - the poi 'bobette' 
         - an adresse in bob's street that is not in the dataset
        """
        args = {
            u'pt_dataset': 'main_routing_test',
            u'type[]': [u'public_transport:stop_area', u'street', u'house', u'poi', u'city'],
            u'limit': 10
        }
        params = urlencode(args, doseq=True)
        mock_requests = MockRequests({
            'https://host_of_bragi/autocomplete?q=bobette&{p}'.format(p=params): (BRAGI_MOCK_BOBETTE, 200),
            'https://host_of_bragi/features/bobette?pt_dataset=main_routing_test': (BRAGI_MOCK_BOBETTE, 200),
            'https://host_of_bragi/autocomplete?q=20+rue+bob&{p}'.format(p=params): (BOB_STREET, 200),
            'https://host_of_bragi/reverse?lat={lat}&lon={lon}&pt_dataset=main_routing_test'
            .format(lon=check_utils.r_coord.split(';')[0], lat=check_utils.r_coord.split(';')[1])
            : (BOB_STREET, 200)
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
                assert response_from['id'] == "bobette"
                assert response_from['name'] == "bobette's label"
                assert response_from['embedded_type'] == "poi"
                assert response_from['poi']['label'] == "bobette's label"
                assert response_from['poi']['properties']["amenity"] == "bicycle_rental"
                assert response_from['poi']['properties']["capacity"] == "20"
                assert response_from['poi']['properties']["ref"] == "12"
                assert response_from['poi']['address']['id'] == "8.98312e-05;8.98312e-05"
                assert response_from['poi']['address']['name'] == "Speloncato-Monticello"
                assert response_from['poi']['address']['label'] == "Speloncato-Monticello (Speloncato)"
                assert response_from['poi']['address']['house_number'] == 0
                assert response_from['poi']['address']['coord']['lat'] == '8.98312e-05'
                assert response_from['poi']['address']['coord']['lon'] == '8.98312e-05'
                assert not response_from['poi']['address'].get('administrative_regions')
                response_to = j['sections'][-1]['to']
                assert response_to['id'] == journeys_to
                assert response_to['name'] == "20 Rue Bob (Bobtown)"
                assert response_to['embedded_type'] == "address"
                assert response_to['address']['label'] == "20 Rue Bob (Bobtown)"

    def test_global_coords_uri(self):
        url = 'https://host_of_bragi'
        params = {
            'pt_dataset': 'main_routing_test',
            'lon': 3.282103,
            'lat': 49.84758
        }

        url += "/reverse?{}".format(urlencode(params, doseq=True))

        mock_requests = MockRequests({
            url: (BRAGI_MOCK_RESPONSE, 200)
        })

        with mock.patch('requests.get', mock_requests.get):
            response = self.query("/v1/coverage/{pt_dataset}/coords/{lon};{lat}".
                                  format(lon=params.get('lon'), lat=params.get('lat'),
                                         pt_dataset=params.get('pt_dataset')))

            address = response.get('address')
            assert address
            assert address['house_number'] == 20
            assert address['name'] == 'Rue Bob'
            assert address['label'] == '20 Rue Bob (Bobtown)'
            assert len(address['administrative_regions']) == 1


@config({'scenario': 'new_default'})
class TestNewDefaultAutocompleteAndRouting(AbstractAutocompleteAndRouting,
                                           NewDefaultScenarioAbstractTestFixture):
    pass


@config({'scenario': 'distributed'})
class TestDistributedAutocompleteAndRouting(AbstractAutocompleteAndRouting,
                                         NewDefaultScenarioAbstractTestFixture):
    pass
