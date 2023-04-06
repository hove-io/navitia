# coding=utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
import mock
from pytest import raises

from jormungandr.tests.utils_test import user_set, FakeUser
from tests.check_utils import is_valid_global_autocomplete
from tests import check_utils
from tests.tests_mechanism import NewDefaultScenarioAbstractTestFixture
from .tests_mechanism import AbstractTestFixture, dataset
from jormungandr import app
from six.moves.urllib.parse import urlencode
from .tests_mechanism import config
from copy import deepcopy
import requests_mock
from contextlib import contextmanager
import json


class FakeUserBragi(FakeUser):
    @classmethod
    def get_from_token(cls, token, valid_until):
        """
        Create an empty user
        """
        return user_in_db_bragi[token]


def geojson():
    return (
        '{"type": "Feature", "geometry": '
        '{"type": "Point", "coordinates": [102.0, 0.5]}, "properties": {"prop0": "value0"}}'
    )


user_in_db_bragi = {
    'test_user_no_shape': FakeUserBragi(
        'test_user_no_shape', 1, have_access_to_free_instances=False, is_super_user=True
    ),
    'test_user_with_shape': FakeUserBragi('test_user_with_shape', 2, True, False, False, shape=geojson()),
    'test_user_with_coord': FakeUserBragi('test_user_with_coord', 2, True, False, False, default_coord="12;42"),
}


MOCKED_INSTANCE_CONF = {'instance_config': {'default_autocomplete': 'bragi'}}

MOCKED_INSTANCE_POI_DATASET_CONF = {
    'instance_config': {'default_autocomplete': 'bragi', 'poi_dataset': 'priv.bob'}
}

BRAGI_MOCK_RESPONSE = {
    "features": [
        {
            "geometry": {"coordinates": [3.282103, 49.847586], "type": "Point"},
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
                            "coord": {"lat": 48.8396154, "lon": 2.3957517},
                        }
                    ],
                }
            },
            "type": "Feature",
            "distance": 400,
        }
    ]
}

BRAGI_MOCK_REVERSE_RESPONSE_NEW_ID_FMT = {
    "features": [
        {
            "geometry": {"coordinates": [3.282103, 49.847586], "type": "Point"},
            "properties": {
                "geocoding": {
                    "city": "Bobtown",
                    "housenumber": "20",
                    "id": "49.847586;3.282103-12",  # <- the id has a new format
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
                            "coord": {"lat": 48.8396154, "lon": 2.3957517},
                        }
                    ],
                }
            },
            "type": "Feature",
            "distance": 400,
        }
    ]
}

BRAGI_BOB_STREET_NEW_ID_FMT = {
    "features": [
        {
            "geometry": {"coordinates": [0.00188646, 0.00071865], "type": "Point"},
            "properties": {
                "geocoding": {
                    "city": "Bobtown",
                    "housenumber": "20",
                    "id": "addr:"
                    + check_utils.r_coord
                    + "-random-postfix",  # the id has a postfix, it should not be a pb
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
                            "coord": {"lat": 48.8396154, "lon": 2.3957517},
                        }
                    ],
                }
            },
            "type": "Feature",
        }
    ]
}


BRAGI_MOCK_ZONE = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [2.3514616, 48.8566969], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": "admin:fr:75056",
                    "type": "zone",
                    "zone_type": "city",
                    "label": "Paris (75000-75116), Île-de-France, France",
                    "name": "Paris",
                    "postcode": "75000;75001;75002;75003;75004;75005;75006;75007;75008;75009;75010;75011;75012;75013;75014;75015;75016;75017;75018;75019;75020;75116",
                    "city": None,
                    "citycode": "75056",
                    "level": 8,
                    "administrative_regions": [],
                    "codes": [{"name": "ref:FR:MGP", "value": "T1"}, {"name": "ref:INSEE", "value": "75056"}],
                    "bbox": [2.224122, 48.8155755, 2.4697602, 48.902156],
                }
            },
        },
        {
            "type": "Feature",
            "geometry": {"coordinates": [2.374402147020069, 48.84691600012601], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": "admin:fr:7511248",
                    "type": "zone",
                    "zone_type": "suburb",
                    "label": "Quartier des Quinze-Vingts (75012), Paris 12e Arrondissement, Paris, Île-de-France, France",
                    "name": "Quartier des Quinze-Vingts",
                    "postcode": "75012",
                    "city": None,
                    "citycode": "7511248",
                    "level": 10,
                    "administrative_regions": [],
                    "codes": [{"name": "ref:INSEE", "value": "7511248"}],
                    "bbox": [2.3644295, 48.8401716, 2.3843422, 48.853255399999995],
                }
            },
        },
    ]
}


BRAGI_MOCK_TYPE_UNKNOWN = {
    "type": "FeatureCollection",
    "geocoding": {"version": "0.1.0", "query": ""},
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [3.1092154, 50.6274528], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": "admin:osm:relation:2643160",
                    "type": "unknown",
                    "label": "Hellemmes-Lille",
                    "name": "Hellemmes-Lille",
                    "postcode": None,
                    "city": None,
                    "citycode": "",
                    "level": 9,
                    "administrative_regions": [],
                }
            },
        },
        {
            "type": "Feature",
            "geometry": {"coordinates": [3.0706414, 50.6305089], "type": "Point"},
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
                    "administrative_regions": [],
                }
            },
        },
    ],
}


BRAGI_MOCK_POI_WITHOUT_ADDRESS = {
    "features": [
        {
            "geometry": {"coordinates": [0.0000898312, 0.0000898312], "type": "Point"},
            "properties": {
                "geocoding": {
                    "city": "Bobtown",
                    "id": "bobette",
                    "label": "bobette's label",
                    "name": "bobette",
                    "poi_types": [{"id": "poi_type:amenity:bicycle_rental", "name": "Station VLS"}],
                    "postcode": "02100",
                    "type": "poi",
                    "citycode": "02000",
                    "properties": [
                        {"key": "amenity", "value": "bicycle_rental"},
                        {"key": "capacity", "value": "20"},
                        {"key": "ref", "value": "12"},
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
                            "coord": {"lat": 48.8396154, "lon": 2.3957517},
                        }
                    ],
                }
            },
            "type": "Feature",
            "distance": 400,
        }
    ]
}

BRAGI_MOCK_STOP_AREA_WITH_MORE_ATTRIBUTS = {
    "features": [
        {
            "geometry": {"coordinates": [0.0000898312, 0.0000898312], "type": "Point"},
            "properties": {
                "geocoding": {
                    "city": "Bobtown",
                    "id": "bobette",
                    "label": "bobette's label",
                    "name": "bobette",
                    "poi_types": [{"id": "poi_type:amenity:bicycle_rental", "name": "Station VLS"}],
                    "postcode": "02100",
                    "type": "public_transport:stop_area",
                    "citycode": "02000",
                    "properties": [
                        {"key": "name", "value": "railway station"},
                        {"key": "code", "value": "station:01"},
                    ],
                    "commercial_modes": [
                        {"id": "cm_id:Bus", "name": "cm_name:Bus"},
                        {"id": "cm_id:Car", "name": "cm_name:Car"},
                    ],
                    "physical_modes": [
                        {"id": "pm_id:Bus", "name": "pm_name:Bus"},
                        {"id": "pm_id:Car", "name": "pm_name:Car"},
                    ],
                    "lines": [
                        {
                            "commercial_mode": {"id": "Metro", "name": "Metro"},
                            "id": "M1",
                            "name": "Métro 1",
                            "code": "Métro 1",
                            "network": {"id": "TGN", "name": "The Great Network"},
                            "physical_modes": [{"id": "Metro", "name": "Metro"}],
                            "text_color": "FFFFFF",
                            "color": "7D36F5",
                        },
                        {
                            "commercial_mode": {"id": "Bus", "name": "Bus"},
                            "id": "B5",
                            "name": "Bus 5",
                            "code": "5",
                            "network": {"id": "TGN", "name": "The Great Network"},
                            "physical_modes": [{"id": "Bus", "name": "Bus"}],
                            "color": "7D36F5",
                            "text_color": "FFFFFF",
                        },
                        {
                            "commercial_mode": {"id": "Bus", "name": "Bus"},
                            "id": "B42",
                            "name": "Bus 42",
                            "network": {"id": "TGN", "name": "The Great Network"},
                            "physical_modes": [{"id": "Bus", "name": "Bus"}],
                        },
                    ],
                    "codes": [{"name": "navitia1", "value": "424242"}, {"name": "source", "value": "1161"}],
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
                            "coord": {"lat": 48.8396154, "lon": 2.3957517},
                        }
                    ],
                    "feed_publishers": [
                        {
                            "id": "bob_publisher",
                            "license": "OBobDL",
                            "name": "Bobenstreetmap",
                            "url": "https://www.Bobenstreetmap.org/copyright",
                        }
                    ],
                }
            },
            "type": "Feature",
            "distance": 400,
        }
    ]
}

BRAGI_MOCK_STOP_AREA_WITH_BASIC_ATTRIBUTS = {
    "features": [
        {
            "geometry": {
                "coordinates": [0.0000898312, 0.0000898312],  # has to match with kraken mock
                "type": "Point",
            },
            "properties": {
                "geocoding": {
                    "city": "Bobtown",
                    "id": "bobette",
                    "label": "bobette's label",
                    "name": "bobette",
                    "poi_types": [{"id": "poi_type:amenity:bicycle_rental", "name": "Station VLS"}],
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
                            "coord": {"lat": 48.8396154, "lon": 2.3957517},
                        }
                    ],
                }
            },
            "type": "Feature",
        }
    ]
}

BRAGI_MOCK_BOBETTE = {
    "features": [
        {
            "geometry": {"coordinates": [0.0000898312, 0.0000898312], "type": "Point"},
            "properties": {
                "geocoding": {
                    "city": "Bobtown",
                    "id": "bobette",
                    "label": "bobette's label",
                    "name": "bobette",
                    "poi_types": [{"id": "poi_type:amenity:bicycle_rental", "name": "Station VLS"}],
                    "postcode": "02100",
                    "type": "poi",
                    "citycode": "02000",
                    "properties": [
                        {"key": "amenity", "value": "bicycle_rental"},
                        {"key": "capacity", "value": "20"},
                        {"key": "ref", "value": "12"},
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
                                "coord": {"lat": 48.8396154, "lon": 2.3957517},
                            }
                        ],
                        "weight": 0.00847457627118644,
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
                            "coord": {"lat": 48.8396154, "lon": 2.3957517},
                        }
                    ],
                }
            },
            "type": "Feature",
        }
    ]
}
# Original coordinates are used to calculate id. We need different lat and lon for test
BRAGI_MOCK_BOBETTE_DEPTH_ZERO = deepcopy(BRAGI_MOCK_BOBETTE)
BRAGI_MOCK_BOBETTE_DEPTH_ZERO["features"][0]["geometry"] = {
    "coordinates": [2.3957517, 48.8396154],
    "type": "Point",
}
BRAGI_MOCK_BOBETTE_DEPTH_ONE = deepcopy(BRAGI_MOCK_BOBETTE_DEPTH_ZERO)
BRAGI_MOCK_BOBETTE_DEPTH_TWO = deepcopy(BRAGI_MOCK_BOBETTE_DEPTH_ZERO)
BRAGI_MOCK_BOBETTE_DEPTH_THREE = deepcopy(BRAGI_MOCK_BOBETTE_DEPTH_ZERO)

BOB_STREET = {
    "features": [
        {
            "geometry": {"coordinates": [0.00188646, 0.00071865], "type": "Point"},
            "properties": {
                "geocoding": {
                    "city": "Bobtown",
                    "housenumber": "20",
                    "id": "addr:" + check_utils.r_coord,  # the adresse is just above 'R'
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
                            "coord": {"lat": 48.8396154, "lon": 2.3957517},
                        }
                    ],
                }
            },
            "type": "Feature",
        }
    ]
}

BRAGI_MOCK_ADMIN = {
    "type": "FeatureCollection",
    "geocoding": {"version": "0.1.0", "query": ""},
    "features": [
        {
            "type": "Feature",
            "distance": 400,
            "geometry": {"coordinates": [5.0414701, 47.3215806], "type": "Point"},
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
                    "administrative_regions": [],
                }
            },
        }
    ],
}

BRAGI_MOCK_ADMINISTRATIVE_REGION = {
    "type": "FeatureCollection",
    "geocoding": {"version": "0.1.0", "query": ""},
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [2.4518371, 48.7830727], "type": "Point"},
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
                    "administrative_regions": [],
                }
            },
        }
    ],
}

BRAGI_MOCK_ADMINISTRATIVE_REGION_WITH_WRONG_TYPE = {
    "type": "FeatureCollection",
    "geocoding": {"version": "0.1.0", "query": ""},
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [2.4518371, 48.7830727], "type": "Point"},
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
                    "administrative_regions": [],
                }
            },
        }
    ],
}

BRAGI_MOCK_RESPONSE_STOP_AREA_WITH_COMMENTS = {
    "type": "FeatureCollection",
    "geocoding": {"version": "0.1.0", "query": ""},
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [3.282103, 49.847586], "type": "Point"},
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
                    "commercial_modes": [{"id": "commercial_mode:Bus", "name": "Bus"}],
                    "physical_modes": [{"id": "commercial_mode:Bus", "name": "Bus"}],
                    "comments": [{"name": "comment1"}, {"name": "comment2"}],
                    "timezone": "Europe/Paris",
                    "codes": [{"name": "code_name1", "value": "1"}, {"name": "coce_name2", "value": "2"}],
                    "feed_publishers": [
                        {
                            "id": "feed_p_id",
                            "license": "feed_p_license",
                            "name": "feed_p_name",
                            "url": "feed_p_url",
                        }
                    ],
                }
            },
        }
    ],
}

BRAGI_MOCK_RESPONSE_STOP_AREA_WITHOUT_COMMENTS = {
    "type": "FeatureCollection",
    "geocoding": {"version": "0.1.0", "query": ""},
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [3.282103, 49.847586], "type": "Point"},
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
                    "commercial_modes": [{"id": "commercial_mode:Bus", "name": "Bus"}],
                    "physical_modes": [{"id": "commercial_mode:Bus", "name": "Bus"}],
                    "timezone": "Europe/Paris",
                    "codes": [{"name": "code_name1", "value": "1"}, {"name": "coce_name2", "value": "2"}],
                    "feed_publishers": [
                        {
                            "id": "feed_p_id",
                            "license": "feed_p_license",
                            "name": "feed_p_name",
                            "url": "feed_p_url",
                        }
                    ],
                }
            },
        }
    ],
}

BRAGI_MOCK_RESPONSE_POI_WITH_CHILDREN = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [2.33642846001, 48.8487134929], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": "poi:10309",
                    "type": "poi",
                    "label": "Jardin du Luxembourg (Paris)",
                    "name": "Jardin du Luxembourg",
                    "city": "Paris",
                    "citycode": "75056",
                    "administrative_regions": [
                        {
                            "id": "admin:fr:75056",
                            "insee": "75056",
                            "level": 8,
                            "label": "Paris (75000-75116), Île-de-France, France",
                            "name": "Paris",
                            "coord": {"lon": 2.3483915, "lat": 48.8534951},
                            "zone_type": "city",
                            "parent_id": "admin:osm:relation:71525",
                        }
                    ],
                    "poi_types": [{"id": "poi_type:jardin", "name": "Jardin"}],
                    "children": [
                        {
                            "type": "poi",
                            "id": "poi:osm:node:4507085760",
                            "label": "Jardin du Luxembourg: Porte Vavin (Paris)",
                            "name": "Jardin du Luxembourg: Porte Vavin",
                            "coord": {"lon": 2.3324915, "lat": 48.8448854},
                            "administrative_regions": [
                                {
                                    "type": "admin",
                                    "id": "admin:fr:75056",
                                    "insee": "75056",
                                    "level": 8,
                                    "label": "Paris (75000-75116), Île-de-France, France",
                                    "name": "Paris",
                                    "weight": 0.0015625185714285715,
                                    "coord": {"lon": 2.3483915, "lat": 48.8534951},
                                    "administrative_regions": [],
                                    "zone_type": "city",
                                    "parent_id": "admin:osm:relation:71525",
                                    "country_codes": [],
                                    "names": {"fr": "Paris"},
                                    "labels": {},
                                }
                            ],
                            "weight": 0.0015625185714285715,
                            "zip_codes": [],
                            "poi_type": {"id": "poi_type:access_point", "name": "Point d'accès"},
                            "properties": {},
                            "country_codes": ["FR"],
                            "names": {},
                            "labels": {},
                            "children": [],
                        },
                        {
                            "type": "poi",
                            "id": "poi:osm:node:790012494",
                            "label": "Jardin du Luxembourg: Porte Croquet (Paris)",
                            "name": "Jardin du Luxembourg: Porte Croquet",
                            "coord": {"lon": 2.3325078, "lat": 48.845564},
                            "administrative_regions": [
                                {
                                    "type": "admin",
                                    "id": "admin:fr:75056",
                                    "insee": "75056",
                                    "level": 8,
                                    "label": "Paris (75000-75116), Île-de-France, France",
                                    "name": "Paris",
                                    "weight": 0.0015625185714285715,
                                    "coord": {"lon": 2.3483915, "lat": 48.8534951},
                                    "administrative_regions": [],
                                    "zone_type": "city",
                                    "parent_id": "admin:osm:relation:71525",
                                    "country_codes": [],
                                    "names": {"fr": "Paris"},
                                    "labels": {},
                                }
                            ],
                            "weight": 0.0015625185714285715,
                            "poi_type": {"id": "poi_type:access_point", "name": "Point d'accès"},
                            "properties": {},
                            "address": {
                                "type": "addr",
                                "id": "addr:2.332236;48.845573:38",
                                "name": "38 Rue Guynemer",
                                "house_number": "38",
                                "street": {
                                    "type": "street",
                                    "id": "street:751064442R",
                                    "name": "Rue Guynemer",
                                    "label": "Rue Guynemer (Paris)",
                                    "weight": 0.0015625185714285715,
                                    "coord": {"lon": 2.332236, "lat": 48.845573},
                                    "zip_codes": ["75006"],
                                    "country_codes": ["fr"],
                                },
                                "label": "38 Rue Guynemer (Paris)",
                                "coord": {"lon": 2.332236, "lat": 48.845573},
                                "weight": 0.0015625185714285715,
                                "zip_codes": ["75006"],
                            },
                            "names": {},
                            "labels": {},
                            "children": [],
                        },
                    ],
                    "address": {
                        "id": "street:osm:way:261896959",
                        "type": "street",
                        "label": "Cour de Jonction (Paris)",
                        "name": "Cour de Jonction",
                        "street": "Cour de Jonction",
                        "postcode": "75006",
                        "city": "Paris",
                        "citycode": "75056",
                        "administrative_regions": [
                            {
                                "id": "admin:fr:75056",
                                "insee": "75056",
                                "level": 8,
                                "label": "Paris (75000-75116), Île-de-France, France",
                                "name": "Paris",
                                "coord": {"lon": 2.3483915, "lat": 48.8534951},
                                "zone_type": "city",
                                "parent_id": "admin:osm:relation:71525",
                            }
                        ],
                        "country_codes": ["FR"],
                    },
                    "country_codes": ["FR"],
                }
            },
        }
    ]
}

BRAGI_MOCK_RESPONSE_POI_WITHOUT_CHILDREN = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [2.33642846001, 48.8487134929], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": "poi:10309",
                    "type": "poi",
                    "label": "Jardin du Luxembourg (Paris)",
                    "name": "Jardin du Luxembourg",
                    "city": "Paris",
                    "citycode": "75056",
                    "administrative_regions": [
                        {
                            "id": "admin:fr:75056",
                            "insee": "75056",
                            "level": 8,
                            "label": "Paris (75000-75116), Île-de-France, France",
                            "name": "Paris",
                            "coord": {"lon": 2.3483915, "lat": 48.8534951},
                            "zone_type": "city",
                            "parent_id": "admin:osm:relation:71525",
                        }
                    ],
                    "poi_types": [{"id": "poi_type:jardin", "name": "Jardin"}],
                    "address": {
                        "id": "street:osm:way:261896959",
                        "type": "street",
                        "label": "Cour de Jonction (Paris)",
                        "name": "Cour de Jonction",
                        "street": "Cour de Jonction",
                        "postcode": "75006",
                        "city": "Paris",
                        "citycode": "75056",
                        "administrative_regions": [
                            {
                                "id": "admin:fr:75056",
                                "insee": "75056",
                                "level": 8,
                                "label": "Paris (75000-75116), Île-de-France, France",
                                "name": "Paris",
                                "coord": {"lon": 2.3483915, "lat": 48.8534951},
                                "zone_type": "city",
                                "parent_id": "admin:osm:relation:71525",
                            }
                        ],
                        "country_codes": ["FR"],
                    },
                    "country_codes": ["FR"],
                }
            },
        }
    ]
}


@contextmanager
def mock_bragi_autocomplete_call(bragi_response, limite=10, http_response_code=200, method='POST'):
    url = 'https://host_of_bragi/autocomplete'
    params = [
        ('q', u'bob'),
        ('type[]', u'public_transport:stop_area'),
        ('type[]', u'street'),
        ('type[]', u'house'),
        ('type[]', u'poi'),
        ('type[]', u'city'),
        ('limit', limite),
        ('pt_dataset[]', 'main_routing_test'),
        ('timeout', 2000),
    ]
    params.sort()

    url += "?{}".format(urlencode(params, doseq=True))

    with requests_mock.Mocker() as m:
        m.request(method, url, json=bragi_response)
        yield m


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class TestBragiAutocomplete(AbstractTestFixture):
    def test_autocomplete_no_json(self):
        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', text="this isn't a json")
            resp, status = self.query_region('places?q=bob&from=3.25;49.84', check=False)
            assert status == 500
            assert m.called

    def test_autocomplete_call(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_RESPONSE):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region"
            )

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['id'] == '3.282103;49.847586'
            assert r[0]['name'] == '20 Rue Bob (Bobtown)'
            assert r[0]['embedded_type'] == 'address'
            assert r[0]['address']['name'] == 'Rue Bob'
            assert r[0]['address']['label'] == '20 Rue Bob (Bobtown)'
            assert r[0]['distance'] == '400'
            fbs = response['feed_publishers']
            assert {fb['id'] for fb in fbs} >= {u'osm', u'bano'}
            assert len(r[0]['address'].get('administrative_regions')) == 1

    def test_autocomplete_call_depth_zero(self):
        with mock_bragi_autocomplete_call(deepcopy(BRAGI_MOCK_RESPONSE)):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&timeout=2000&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region&depth=0"
            )

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
        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', json={})
            self.query_region('places?q=bob&from=3.25;49.84')
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert params.get('lon') == ['3.25']
            assert params.get('lat') == ['49.84']
            assert params.get('timeout') == ['2000']

    def test_autocomplete_call_with_param_places_proximity_radius(self):
        """
        test that the from param of the autocomplete is correctly given to bragi
        :return:
        """
        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', json={})
            self.query_region('places?q=bob&from=3.25;49.84&places_proximity_radius=10000')
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert params.get('lon') == ['3.25']
            assert params.get('lat') == ['49.84']
            assert params.get('timeout') == ['2000']
            assert params.get('proximity_offset') == ['10.0']
            assert params.get('proximity_scale') == ['65.0']  # 10 * 6.5
            assert params.get('proximity_decay') == ['0.4']

    def test_autocomplete_call_override(self):
        """
        test that the _autocomplete param switch the right autocomplete service
        """
        with mock_bragi_autocomplete_call(BRAGI_MOCK_RESPONSE):
            response = self.query_region(
                "places?q=bob&type[]=stop_area&type[]=address&type[]=poi&type[]=administrative_region"
            )

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
        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', json={})
            self.query_region('places?q=bob')
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert 'type[]' in params
            assert set(params['type[]']) == set(['public_transport:stop_area', 'street', 'house', 'poi', 'city'])

    def test_autocomplete_call_with_no_shape(self):
        """
        test that the coverage shape is sent to bragi if no shape is set
        :return:
        """
        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', json={})
            self.query_region('places?q=bob')
            assert m.called
            body = m.request_history[0].body
            assert body
            js = json.loads(body)
            shape = js.get('shape')
            assert shape
            type = shape.get('type')
            assert type == 'Feature'
            geom = shape.get('geometry')
            assert geom
            assert geom.get('type') == 'Polygon'
            coords = [
                [
                    [-0.99991, -0.99991],
                    [-0.99991, 0.00171865],
                    [0.00288646, 0.00171865],
                    [0.00288646, -0.99991],
                    [-0.99991, -0.99991],
                ]
            ]
            assert geom.get('coordinates') == coords

    def test_autocomplete_call_with_param_type_administrative_region(self):
        """
        test that administrative_region is converted to city
        :return:
        """
        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', json={})
            self.query_region('places?q=bob&type[]=administrative_region&type[]=address')
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert 'type[]' in params
            assert set(params['type[]']) == set(['city', 'street', 'house'])

    def test_autocomplete_call_with_param_type_not_acceptable(self):
        """
        test not acceptable type
        :return:
        """
        with raises(Exception):
            with requests_mock.Mocker() as m:
                m.post('https://host_of_bragi/autocomplete', status_code=422)
                self.query_region('places?q=bob&type[]=bobette')

    def test_autocomplete_call_with_param_type_stop_point(self):
        """
        test that stop_point is not passed to bragi
        :return:
        """
        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', json={})
            self.query_region('places?q=bob&type[]=stop_point&type[]=address')
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert 'type[]' in params
            assert set(params['type[]']) == set(['street', 'house'])

    def test_features_call(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}

        url += "/features/1234?{}".format(urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_RESPONSE)
            response = self.query_region("places/1234?&pt_dataset[]=main_routing_test")
            assert m.called

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == '20 Rue Bob (Bobtown)'
            assert r[0]['embedded_type'] == 'address'
            assert r[0]['address']['name'] == 'Rue Bob'
            assert r[0]['address']['label'] == '20 Rue Bob (Bobtown)'

    def test_features_unknown_uri(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}

        url += "/features/AAA?{}".format(urlencode(params, doseq=True))
        response = {'short': 'query error', 'long': 'invalid query EsError("Unable to find object")'}
        with requests_mock.Mocker() as m:
            m.get(url, json=response, status_code=404)
            response = self.query_region("places/AAA?&pt_dataset[]=main_routing_test", check=False)
            assert m.called
            assert response[1] == 404
            assert response[0]["error"]["id"] == 'unknown_object'
            assert response[0]["error"]["message"] == "The object AAA doesn't exist"

    def test_poi_without_address(self):
        url = 'https://host_of_bragi'
        params = {'pt_dataset[]': 'main_routing_test', 'timeout': 200}

        url += "/features/1234?{}".format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_POI_WITHOUT_ADDRESS)
            response = self.query_region("places/1234?&pt_dataset[]=main_routing_test")
            assert m.called

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['id'] == 'bobette'
            assert r[0]['embedded_type'] == 'poi'
            assert r[0]['poi']['name'] == 'bobette'
            assert r[0]['poi']['label'] == "bobette's label"
            assert r[0]['distance'] == '400'
            assert not r[0]['poi'].get('address')

    def test_stop_area_with_modes(self):
        url = 'https://host_of_bragi'
        params = {'pt_dataset[]': 'main_routing_test', 'timeout': 200}

        url += "/features/1234?{}".format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_STOP_AREA_WITH_MORE_ATTRIBUTS)
            response = self.query_region("places/1234?&pt_dataset[]=main_routing_test")
            assert m.called

            assert response.get('feed_publishers')
            assert len(response.get('feed_publishers')) == 3

            r = response.get('places')
            assert len(r) == 1
            sa = r[0]['stop_area']
            assert r[0]['embedded_type'] == 'stop_area'
            assert sa['name'] == 'bobette'
            assert len(sa.get('commercial_modes')) == 2
            assert sa.get('commercial_modes')[0].get('id') == 'cm_id:Bus'
            assert sa.get('commercial_modes')[0].get('name') == 'cm_name:Bus'
            assert sa.get('commercial_modes')[1].get('id') == 'cm_id:Car'
            assert sa.get('commercial_modes')[1].get('name') == 'cm_name:Car'

            assert len(sa.get('physical_modes')) == 2
            assert sa.get('physical_modes')[0].get('id') == 'pm_id:Bus'
            assert sa.get('physical_modes')[0].get('name') == 'pm_name:Bus'
            assert sa.get('physical_modes')[1].get('id') == 'pm_id:Car'
            assert sa.get('physical_modes')[1].get('name') == 'pm_name:Car'

            assert len(sa.get('lines')) == 3
            assert sa.get('lines')[0].get('id') == 'M1'
            assert sa.get('lines')[0].get('name') == 'Métro 1'
            assert sa.get('lines')[0].get('code') == 'Métro 1'
            assert sa.get('lines')[0].get('commercial_mode') == {'id': 'Metro', 'name': 'Metro'}
            assert sa.get('lines')[0].get('network') == {'id': 'TGN', 'name': 'The Great Network'}
            assert sa.get('lines')[0].get('physical_modes') == [{'id': 'Metro', 'name': 'Metro'}]
            assert sa.get('lines')[0].get('text_color') == "FFFFFF"
            assert sa.get('lines')[0].get('color') == "7D36F5"

            assert sa.get('lines')[1].get('id') == 'B5'
            assert sa.get('lines')[1].get('name') == 'Bus 5'
            assert sa.get('lines')[1].get('code') == '5'
            assert sa.get('lines')[1].get('commercial_mode') == {'id': 'Bus', 'name': 'Bus'}
            assert sa.get('lines')[1].get('network') == {'id': 'TGN', 'name': 'The Great Network'}
            assert sa.get('lines')[1].get('physical_modes') == [{'id': 'Bus', 'name': 'Bus'}]
            assert sa.get('lines')[1].get('text_color') == "FFFFFF"
            assert sa.get('lines')[1].get('color') == "7D36F5"

            assert len(sa.get('codes')) == 2
            assert sa.get('codes')[0].get('type') == 'external_code'
            assert sa.get('codes')[0].get('value') == '424242'
            assert sa.get('codes')[1].get('type') == 'source'
            assert sa.get('codes')[1].get('value') == '1161'

            assert sa.get('properties').get('name') == 'railway station'
            assert sa.get('properties').get('code') == 'station:01'

            assert sa.get('timezone') == 'Europe/Paris'
            admins = sa.get('administrative_regions')
            assert r[0]['distance'] == '400'
            assert len(admins) == 1

    def test_stop_area_with_modes_depth_zero(self):
        with mock_bragi_autocomplete_call(deepcopy(BRAGI_MOCK_STOP_AREA_WITH_MORE_ATTRIBUTS)):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region&depth=0"
            )

            assert response.get('feed_publishers')
            assert len(response.get('feed_publishers')) == 3

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
        params = {'pt_dataset[]': 'main_routing_test', 'timeout': 200}

        url += "/features/1234?{}".format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_STOP_AREA_WITH_BASIC_ATTRIBUTS)
            response = self.query_region("places/1234?&pt_dataset[]=main_routing_test")

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
            assert 'distance' not in r[0]

    def test_feature_unknown_type(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_TYPE_UNKNOWN, limite=2, method='GET'):
            response = self.query("v1/places?q=bob&count=2")

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            # check that we get only one response, the other one being filtered as type is unknown
            assert len(r) == 1
            assert r[0]['name'] == 'Lille'
            assert r[0]['embedded_type'] == 'administrative_region'
            assert r[0]['id'] == 'admin:fr:59350'
            assert r[0]['administrative_region']['label'] == 'Lille (59000-59800)'

    def test_feature_zone(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_ZONE, method='GET'):
            response = self.query("v1/places?q=bob")

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            # The suburb is currently ignored, only cities are returned
            assert len(r) == 1
            assert r[0]['name'] == 'Paris'
            assert r[0]['embedded_type'] == 'administrative_region'
            assert r[0]['id'] == 'admin:fr:75056'
            assert r[0]['administrative_region']['label'] == 'Paris (75000-75116), Île-de-France, France'

    def test_autocomplete_call_with_depth_zero(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_BOBETTE_DEPTH_ZERO):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region&depth=0"
            )

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
            # Empty administrative_regions not displayed as in kraken
            assert not poi.get('administrative_regions')
            assert 'administrative_regions' not in poi
            # Address absent as in kraken
            assert 'address' not in poi

    def test_autocomplete_call_with_depth_one(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_BOBETTE_DEPTH_ONE):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region&depth=1"
            )

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
            # Empty administrative_regions not displayed as in kraken
            assert not address.get('administrative_regions')

    def test_autocomplete_call_with_depth_two(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_BOBETTE_DEPTH_TWO):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region&depth=2"
            )

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

    # This test is to verify that query with depth = 2 and 3 gives the same result as in kraken
    def test_autocomplete_call_with_depth_three(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_BOBETTE_DEPTH_THREE):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region&depth=3"
            )

            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == "bobette's label"
            assert r[0]['embedded_type'] == "poi"
            assert 'distance' not in r[0]

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
        with mock_bragi_autocomplete_call(BRAGI_MOCK_ADMIN):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region&depth=0"
            )

            is_valid_global_autocomplete(response, depth=0)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == 'Dijon'
            assert r[0]['embedded_type'] == 'administrative_region'
            assert r[0]['id'] == 'admin:fr:21231'
            assert r[0]['distance'] == '400'
            assert r[0]['administrative_region']['id'] == 'admin:fr:21231'
            assert r[0]['administrative_region']['insee'] == '21231'
            assert r[0]['administrative_region']['label'] == 'Dijon (21000)'
            assert r[0]['administrative_region']['name'] == 'Dijon'
            assert 'administrative_regions' not in r[0]['administrative_region']

    def test_autocomplete_for_administrative_region(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_ADMINISTRATIVE_REGION):
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
            assert 'distance' not in r[0]

    def test_autocomplete_for_administrative_region_with_wrong_type(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_ADMINISTRATIVE_REGION_WITH_WRONG_TYPE):
            response = self.query_region("places?q=bob")
            r = response.get('places')
            assert len(r) == 0

    # Since administrative_regions of the admin is an empty list in the result bragi
    # there is no difference in the final result with depth from 0 to 3
    def test_autocomplete_for_admin_depth_two(self):
        with mock_bragi_autocomplete_call(deepcopy(BRAGI_MOCK_ADMIN)):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region&depth=2"
            )

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
        with mock_bragi_autocomplete_call(BRAGI_MOCK_RESPONSE_STOP_AREA_WITH_COMMENTS):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region"
            )
            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == 'stop 1'
            assert r[0]['embedded_type'] == 'stop_area'
            assert r[0]['id'] == 'stop_area_id'
            assert r[0]['quality'] == 0  # field for kraken compatibility (default = 0)
            stop_area = r[0]['stop_area']
            # For retrocompatibility, when we have multi-comments, we keep 'comment' field with
            # the first comment element.
            assert stop_area['comment'] == 'comment1'
            comments = stop_area['comments']
            assert {comment['value'] for comment in comments} >= {u'comment1', u'comment2'}

    def test_autocomplete_call_without_comments_on_stop_area(self):
        with mock_bragi_autocomplete_call(BRAGI_MOCK_RESPONSE_STOP_AREA_WITHOUT_COMMENTS):
            response = self.query_region(
                "places?q=bob&pt_dataset[]=main_routing_test&type[]=stop_area"
                "&type[]=address&type[]=poi&type[]=administrative_region"
            )
            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == 'stop 1'
            assert r[0]['embedded_type'] == 'stop_area'
            assert r[0]['id'] == 'stop_area_id'
            assert r[0]['quality'] == 0  # field for kraken compatibility (default = 0)
            stop_area = r[0]['stop_area']
            # When no comments exist in Bragi, the API mask the "comment" and "comments" field.
            assert not stop_area.get('comment')
            assert not stop_area.get('comments')

    def test_place_uri_stop_point(self):
        url = "https://host_of_bragi/features/stop_point:stopB?{}".format(
            urlencode({'timeout': 200, 'pt_dataset[]': 'main_routing_test'}, doseq=True)
        )
        response = {'short': 'query error', 'long': 'invalid query EsError("Unable to find object")'}
        with requests_mock.Mocker() as m:
            m.get(url, json=response, status_code=404)
            response = self.query_region("places/stop_point:stopB")

            # bragi should have been called
            assert m.called

            # but since it has not found anything, kraken have been called, and he knows this stop_point
            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['embedded_type'] == 'stop_point'
            assert r[0]['id'] == 'stop_point:stopB'

    def test_place_uri_stop_point_global_endpoint(self):
        url = "https://host_of_bragi/features/stop_point:stopB?{}".format(
            urlencode({'timeout': 200, 'pt_dataset[]': 'main_routing_test'}, doseq=True)
        )
        response = {'short': 'query error', 'long': 'invalid query EsError("Unable to find object")'}
        with requests_mock.Mocker() as m:
            m.get(url, json=response, status_code=404)
            response, status = self.query_no_assert("/v1/places/stop_point:stopB")

            # bragi should have been called
            assert m.called

            # for the global /places endpoint, we do not fallback on kraken, even if a pt_dataset is given,
            # so the object cannot be found
            assert status == 404
            assert response["error"]["id"] == 'unknown_object'
            assert "The object stop_point:stopB doesn't exist" in response["error"]["message"]


@dataset({"main_routing_test": {}}, global_config={'activate_bragi': True})
class TestBragiShape(AbstractTestFixture):
    def test_places_for_user_with_shape(self):
        """
        Test that with a shape on user, it is correctly posted
        """
        with user_set(app, FakeUserBragi, "test_user_with_shape"):
            with requests_mock.Mocker() as m:
                m.post('https://host_of_bragi/autocomplete', json={})
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi')
                assert m.called
                self.query('v1/places?q=toto')
                assert m.call_count == 2

            # test that the shape is posted
            def check_shape(m):
                json = m.request_history[-1].json()
                assert json['shape']['type'] == 'Feature'
                assert json.get('shape').get('geometry')

            with requests_mock.Mocker() as m:
                m.post('https://host_of_bragi/autocomplete', json={})
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi')
                assert m.called
                check_shape(m)
                self.query('v1/places?q=toto')
                assert m.call_count == 2
                check_shape(m)

    def test_places_for_user_without_shape(self):
        """
        Test that without shape for user, we use the get method
        """
        with user_set(app, FakeUserBragi, "test_user_no_shape"):
            with requests_mock.Mocker() as m:
                m.post('https://host_of_bragi/autocomplete', json={})
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi')
                assert m.called

                m.get('https://host_of_bragi/autocomplete', json={})
                self.query('v1/places?q=toto')
                assert m.call_count == 2

    def test_places_for_user_with_coord(self):
        """
        Test that with a default_coord on user, it is correctly posted
        """
        with user_set(app, FakeUserBragi, "test_user_with_coord"):
            with requests_mock.Mocker() as m:
                m.post('https://host_of_bragi/autocomplete', json={})
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi')
                assert m.called
                params = m.request_history[0].qs
                assert params
                assert params.get('lon') == ['12']
                assert params.get('lat') == ['42']

    def test_places_for_user_with_coord_and_coord_overriden(self):
        """
        Test that with a default_coord on user, if the user gives a coord we use the given coord
        """
        with user_set(app, FakeUserBragi, "test_user_with_coord"):
            with requests_mock.Mocker() as m:
                m.post('https://host_of_bragi/autocomplete', json={})
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi&from=1;2')
                assert m.called
                params = m.request_history[0].qs
                assert params
                assert params.get('lon') == ['1']
                assert params.get('lat') == ['2']

    def test_places_for_user_with_coord_and_coord_overriden_to_null(self):
        """
        Test that with a default_coord on user, if the user gives an empty coord we do not pass a coord
        """
        with user_set(app, FakeUserBragi, "test_user_with_coord"):
            with requests_mock.Mocker() as m:
                m.post('https://host_of_bragi/autocomplete', json={})
                self.query('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi&from=')
                assert m.called
                params = m.request_history[0].qs
                assert params
                assert params.get('lon') is None
                assert params.get('lat') is None

    def test_places_with_empty_coord(self):
        """
        Test that we get an error if we give an empty coord
        (and if there is no user defined coord to override)
        """
        r, s = self.query_no_assert('v1/coverage/main_routing_test/places?q=toto&_autocomplete=bragi&from=')
        assert s == 400
        assert "if 'from' is provided it cannot be null" in r.get('message')

    def test_global_place_uri(self):
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}
        url = 'https://host_of_bragi/features/bob?{}'.format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_RESPONSE)
            response = self.query("/v1/places/bob")
            assert m.called

            is_valid_global_autocomplete(response, depth=1)
            r = response.get('places')
            assert len(r) == 1
            assert r[0]['name'] == '20 Rue Bob (Bobtown)'
            assert r[0]['embedded_type'] == 'address'
            assert r[0]['address']['name'] == 'Rue Bob'
            assert r[0]['address']['label'] == '20 Rue Bob (Bobtown)'

    def test_global_coords_uri(self):
        url = 'https://host_of_bragi'
        params = {'pt_dataset[]': 'main_routing_test', 'lon': 3.282103, 'lat': 49.84758, 'timeout': 200}
        url += "/reverse?{}".format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_RESPONSE)
            response = self.query(
                "/v1/coverage/{pt_dataset}/coords/{lon};{lat}?_autocomplete=bragi".format(
                    lon=params.get('lon'), lat=params.get('lat'), pt_dataset=params.get('pt_dataset[]')
                )
            )
            assert m.called

            address = response.get('address')
            assert address
            assert address['house_number'] == 20
            assert address['name'] == 'Rue Bob'
            assert address['label'] == '20 Rue Bob (Bobtown)'
            assert len(address['administrative_regions']) == 1


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class AbstractAutocompleteAndRouting:
    def test_journey_with_external_uri_from_bragi(self):
        self.abstract_journey_with_external_uri_from_bragi(BOB_STREET)

    def abstract_journey_with_external_uri_from_bragi(self, bragi_bob_reverse_response):
        """
        This test aim to recreate a classic integration

        The user query the instance's autocomplete (which is set up to bragi)
        And then use the autocomplete's response to query for a journey

        For this test we have 2 item in our autocomplete:
         - the poi 'bobette'
         - an adresse in bob's street that is not in the dataset
        """
        args = [
            (u'pt_dataset[]', 'main_routing_test'),
            (u'type[]', u'public_transport:stop_area'),
            (u'type[]', u'street'),
            (u'type[]', u'house'),
            (u'type[]', u'poi'),
            (u'type[]', u'city'),
            (u'limit', 10),
            ('timeout', 2000),
        ]
        args.sort()
        params = urlencode(args, doseq=True)

        features_url = 'https://host_of_bragi/features/bobette?{}'.format(
            urlencode({'timeout': 200, 'pt_dataset[]': 'main_routing_test'}, doseq=True)
        )
        reverse_url = 'https://host_of_bragi/reverse?{}'.format(
            urlencode(
                {
                    'lon': check_utils.r_coord.split(';')[0],
                    'lat': check_utils.r_coord.split(';')[1],
                    'timeout': 200,
                    'pt_dataset[]': 'main_routing_test',
                },
                doseq=True,
            )
        )
        bobette_args = deepcopy(args) + [('q', 'bobette')]
        bobette_args.sort()
        bobette_params = urlencode(bobette_args, doseq=True)

        bob_args = deepcopy(args) + [('q', '20 rue bob')]
        bob_args.sort()
        bob_params = urlencode(bob_args, doseq=True)

        def get_autocomplete(query):
            autocomplete_response = self.query_region(query)
            r = autocomplete_response.get('places')
            assert len(r) == 1
            return r[0]['id']

        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete?{p}'.format(p=bobette_params), json=BRAGI_MOCK_BOBETTE)
            m.post(
                'https://host_of_bragi/autocomplete?{p}'.format(p=bob_params), json=bragi_bob_reverse_response
            )
            m.get(reverse_url, json=bragi_bob_reverse_response)
            m.get(features_url, json=BRAGI_MOCK_BOBETTE)
            journeys_from = get_autocomplete('places?q=bobette')
            journeys_to = get_autocomplete('places?q=20 rue bob')
            assert m.call_count == 2
            query = 'journeys?from={f}&to={to}&datetime={dt}'.format(
                f=journeys_from, to=journeys_to, dt="20120614T080000"
            )
            journeys_response = self.query_region(query)
            assert m.call_count == 4

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
        params = {'pt_dataset[]': 'main_routing_test', 'lon': 3.282103, 'lat': 49.84758, 'timeout': 200}

        url += "/reverse?{}".format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_RESPONSE)
            response = self.query(
                "/v1/coverage/{pt_dataset}/coords/{lon};{lat}".format(
                    lon=params.get('lon'), lat=params.get('lat'), pt_dataset=params.get('pt_dataset[]')
                )
            )

            address = response.get('address')
            assert address
            assert address['house_number'] == 20
            assert address['name'] == 'Rue Bob'
            assert address['label'] == '20 Rue Bob (Bobtown)'
            assert address['coord'] == {'lat': '49.847586', 'lon': '3.282103'}
            assert address['id'] == '3.282103;49.847586'
            assert len(address['administrative_regions']) == 1

    def test_journey_with_external_uri_from_bragi_new_bragi_coord_id_fmt(self):
        """
        Test that navitia's /journey still works if bragi returns addresses with a new id format (not 'addr:{lon};{lat}' anymore)
        It should not be a problem since navitia is not meant to do some tricks to parse the id, but we never know
        """
        self.abstract_journey_with_external_uri_from_bragi(BOB_STREET)

    def test_global_coords_uri_new_bragi_coord_id_fmt(self):
        """
        Test that navitia's /places still works if bragi returns addresses with a new id format (not 'addr:{lon};{lat}' anymore)
        It should not be a problem since navitia is not meant to do some tricks to parse the id, but we never know
        """
        url = 'https://host_of_bragi'
        params = {'pt_dataset[]': 'main_routing_test', 'lon': 3.282103, 'lat': 49.84758, 'timeout': 200}

        url += "/reverse?{}".format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_REVERSE_RESPONSE_NEW_ID_FMT)
            response = self.query(
                "/v1/coverage/{pt_dataset}/coords/{lon};{lat}".format(
                    lon=params.get('lon'), lat=params.get('lat'), pt_dataset=params.get('pt_dataset[]')
                )
            )

            address = response.get('address')
            assert address
            assert address['house_number'] == 20
            assert address['name'] == 'Rue Bob'
            assert address['label'] == '20 Rue Bob (Bobtown)'
            assert address['coord'] == {'lat': '49.847586', 'lon': '3.282103'}
            assert address['id'] == '3.282103;49.847586'
            assert len(address['administrative_regions']) == 1

    def test_geo_status(self):
        """
        geo_status is still not implemented for bragi
        we test the return code and error message
        """
        url = 'https://host_of_bragi/v1/coverage/main_routing_test/_geo_status'

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_REVERSE_RESPONSE_NEW_ID_FMT)
            response, status_code = self.query_no_assert("/v1/coverage/main_routing_test/_geo_status")
            assert status_code == 404
            assert response == {'message': 'geo_status not implemented'}


@config({'scenario': 'new_default'})
class TestNewDefaultAutocompleteAndRouting(
    AbstractAutocompleteAndRouting, NewDefaultScenarioAbstractTestFixture
):
    pass


@config({'scenario': 'distributed'})
class TestDistributedAutocompleteAndRouting(
    AbstractAutocompleteAndRouting, NewDefaultScenarioAbstractTestFixture
):
    pass


@dataset({'main_routing_test': MOCKED_INSTANCE_POI_DATASET_CONF}, global_config={'activate_bragi': True})
class AbstractAutocompletePoiDataset:
    def test_poi_dataset_places(self):

        """
        Basic test for autocompletion with poi_dataset.
        We setup the mock instance to have a poi_dataset 'priv.bob', and then build a query for bragi.
        We verify that the query to bragi correctly adds a poi_dataset[] parameter.
        """

        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', json={})
            self.query_region('places?q=bob&from=3.25;49.84')
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert params.get('poi_dataset[]') == ['priv.bob']

    def test_poi_dataset_places_coverage(self):

        with requests_mock.Mocker() as m:
            m.post('https://host_of_bragi/autocomplete', json={})
            self.query('v1/coverage/main_routing_test/places?q=bob&_autocomplete=bragi')
            assert m.called

            params = m.request_history[0].qs
            assert params
            assert params.get('poi_dataset[]') == ['priv.bob']

    def test_poi_dataset_reverse(self):

        url = 'https://host_of_bragi'
        params = {'pt_dataset[]': 'main_routing_test', 'lon': 3.282103, 'lat': 49.84758, 'timeout': 200}
        url += "/reverse?{}".format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_RESPONSE)
            response = self.query(
                "/v1/coverage/{pt_dataset}/coords/{lon};{lat}".format(
                    lon=params.get('lon'), lat=params.get('lat'), pt_dataset=params.get('pt_dataset[]')
                )
            )
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert params.get('poi_dataset[]') == ['priv.bob']

    def test_poi_with_children(self):

        url = 'https://host_of_bragi'
        params = {'pt_dataset[]': 'main_routing_test', "poi_dataset[]": "priv.bob", 'timeout': 200}
        url += "/features/bobette?{}".format(urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_RESPONSE_POI_WITH_CHILDREN)
            response = self.query(
                "/v1/coverage/{pt_dataset}/places/bobette".format(pt_dataset=params.get('pt_dataset[]'))
            )
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert params.get('poi_dataset[]') == ['priv.bob']

            assert len(response["places"]) == 1
            first_place = response["places"][0]
            assert first_place["id"] == 'poi:10309'
            assert first_place['embedded_type'] == 'poi'
            assert len(first_place['poi']["children"]) == 2
            assert first_place['poi']["children"][0]["id"] == 'poi:osm:node:4507085760'
            assert first_place['poi']["children"][0]["type"] == 'poi'
            assert first_place['poi']["children"][0]["poi_type"]["id"] == 'poi_type:access_point'

            assert first_place['poi']["children"][1]["id"] == 'poi:osm:node:790012494'
            assert first_place['poi']["children"][1]["type"] == 'poi'
            assert first_place['poi']["children"][1]["poi_type"]["id"] == 'poi_type:access_point'

    def test_poi_without_children(self):

        url = 'https://host_of_bragi'
        params = {'pt_dataset[]': 'main_routing_test', "poi_dataset[]": "priv.bob", 'timeout': 200}
        url += "/features/bobette?{}".format(urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(url, json=BRAGI_MOCK_RESPONSE_POI_WITHOUT_CHILDREN)
            response = self.query(
                "/v1/coverage/{pt_dataset}/places/bobette".format(pt_dataset=params.get('pt_dataset[]'))
            )
            assert m.called
            params = m.request_history[0].qs
            assert params
            assert params.get('poi_dataset[]') == ['priv.bob']

            assert len(response["places"]) == 1
            first_place = response["places"][0]
            assert first_place["id"] == 'poi:10309'
            assert first_place['embedded_type'] == 'poi'
            assert 'children' not in first_place

    def test_poi_dataset_journeys(self):

        url = 'https://host_of_bragi'
        param_from = "8.98312e-05;8.98312e-05"
        param_to = "8.99312e-05;8.97312e-05"
        params = {
            'pt_dataset[]': 'main_routing_test',
            'lon': "8.98312e-05",
            'lat': "8.98312e-05",
            'timeout': 200,
        }
        url += "/reverse?{}".format(urlencode(params, doseq=True))

        with requests_mock.Mocker() as m:

            m.get(url, json={})
            query = 'journeys?from={f}&to={to}&datetime={dt}'.format(
                f=param_from, to=param_to, dt="20120614T080000"
            )
            response, status = self.query_region(query, check=False)

            assert m.called

            params = m.request_history[0].qs
            assert params
            assert params.get('poi_dataset[]') == ['priv.bob']
            assert status == 404
            assert response["error"]["id"] == "no_origin_nor_destination"
            assert response["error"]["message"] == "Public transport is not reachable from origin"


@config({'poi_dataset': 'priv.bob'})
class TestPoiDatasetAutocomplete(AbstractAutocompletePoiDataset, NewDefaultScenarioAbstractTestFixture):
    pass
