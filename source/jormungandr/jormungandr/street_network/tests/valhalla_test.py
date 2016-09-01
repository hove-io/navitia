# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
import pytest
from jormungandr.street_network.valhalla import Valhalla
from jormungandr.exceptions import UnableToParse, InvalidArguments
from navitiacommon import type_pb2


class AbstractObject(object):
    pass


def get_pt_object(type, lon, lat):
    pt_object = type_pb2.PtObject()
    pt_object.embedded_type = type
    if type == type_pb2.ADDRESS:
        pt_object.address.coord.lat = lat
        pt_object.address.coord.lon = lon
    return pt_object


def decode_func_test():
    valhalla = Valhalla(instance=None,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    assert valhalla._decode('qyss{Aco|sCF?kBkHeJw[') == [[2.439938, 48.572841],
                                                         [2.439938, 48.572837],
                                                         [2.440088, 48.572891],
                                                         [2.440548, 48.57307]]


def get_speed_func_test():
    instance = AbstractObject()
    instance.walking_speed = 1
    instance.bike_speed = 2
    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    assert valhalla._get_speed('pedestrian') == 3.6 * 1
    assert valhalla._get_speed('bicycle') == 3.6 * 2


def get_costing_options_func_with_empty_costing_options_test():
    instance = AbstractObject()
    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    valhalla.costing_options = None
    assert valhalla._get_costing_options('bib') == None


def get_costing_options_func_with_unkown_costing_options_test():
    instance = AbstractObject()
    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    assert valhalla._get_costing_options('bib') == {'bib': 'bom'}


def get_costing_options_func_test():
    instance = AbstractObject()
    instance.walking_speed = 1
    instance.bike_speed = 2
    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    pedestrian = valhalla._get_costing_options('pedestrian')
    assert len(pedestrian) == 2
    assert 'pedestrian' in pedestrian
    assert 'walking_speed' in pedestrian['pedestrian']
    assert pedestrian['pedestrian']['walking_speed'] == 3.6 * 1
    assert 'bib' in pedestrian

    bicycle = valhalla._get_costing_options('bicycle')
    assert len(bicycle) == 2
    assert 'bicycle' in bicycle
    assert 'cycling_speed' in bicycle['bicycle']
    assert bicycle['bicycle']['cycling_speed'] == 3.6 * 2
    assert 'bib' in bicycle


def format_coord_func_invalid_pt_object_test():
    instance = AbstractObject()
    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    with pytest.raises(InvalidArguments) as excinfo:
        valhalla._format_coord(AbstractObject())
    assert '400: Bad Request' in str(excinfo.value)


def format_coord_func_valid_coord_test():
    instance = AbstractObject()
    pt_object = get_pt_object(type_pb2.ADDRESS, 1.12, 13.15)
    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})

    coord = valhalla._format_coord(pt_object)
    coord_res = {'lat': pt_object.address.coord.lat, 'type': 'break', 'lon': pt_object.address.coord.lon}
    assert len(coord) == 3
    for key, value in coord_res.items():
        assert coord[key] == value


def format_url_func_with_walking_mode_test():
    instance = AbstractObject()
    instance.walking_speed = 1
    instance.bike_speed = 2
    origin = get_pt_object(type_pb2.ADDRESS, 1.0, 1.0)
    destination = get_pt_object(type_pb2.ADDRESS, 2.0, 2.0)
    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    assert valhalla._format_url("walking", origin,
                                destination) == 'http://bob.com/route?json=' \
                                                '{"costing_options": {"pedestrian": ' \
                                                '{"walking_speed": 3.6}, "bib": "bom"}, "locations": ' \
                                                '[{"lat": 1.0, "type": "break", "lon": 1.0}, ' \
                                                '{"lat": 2.0, "type": "break", "lon": 2.0}], ' \
                                                '"costing": "pedestrian", ' \
                                                '"directions_options": {"units": "kilometers", "bob": "pib"}}&' \
                                                'api_key=None'


def format_url_func_with_bike_mode_test():
    instance = AbstractObject()
    instance.walking_speed = 1
    instance.bike_speed = 2

    origin = get_pt_object(type_pb2.ADDRESS, 1.0, 1.0)
    destination = get_pt_object(type_pb2.ADDRESS, 2.0, 2.0)

    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    valhalla.costing_options = None
    assert valhalla._format_url("bike", origin,
                                destination) == 'http://bob.com/route?json={"costing_options": ' \
                                                '{"bicycle": {"cycling_speed": 7.2}}, ' \
                                                '"locations": [{"lat": 1.0, "type": "break", "lon": 1.0}, ' \
                                                '{"lat": 2.0, "type": "break", "lon": 2.0}], ' \
                                                '"costing": "bicycle", ' \
                                                '"directions_options": {"units": "kilometers", "bob": "pib"}}&' \
                                                'api_key=None'


def format_url_func_with_car_mode_test():
    instance = AbstractObject()
    instance.walking_speed = 1
    instance.bike_speed = 2

    origin = get_pt_object(type_pb2.ADDRESS, 1.0, 1.0)
    destination = get_pt_object(type_pb2.ADDRESS, 2.0, 2.0)

    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    valhalla.costing_options = None
    assert valhalla._format_url("car", origin,
                                destination) == 'http://bob.com/route?json={"locations": ' \
                                                '[{"lat": 1.0, "type": "break", "lon": 1.0}, ' \
                                                '{"lat": 2.0, "type": "break", "lon": 2.0}], ' \
                                                '"costing": "auto", ' \
                                                '"directions_options": {"units": "kilometers", "bob": "pib"}}&' \
                                                'api_key=None'


def format_url_func_invalid_mode_test():
    instance = AbstractObject()
    valhalla = Valhalla(instance=instance,
                        service_url='http://bob.com',
                        directions_options={'bob': 'pib'},
                        costing_options={'bib': 'bom'})
    with pytest.raises(InvalidArguments) as excinfo:
        origin = get_pt_object(type_pb2.ADDRESS, 1.0, 1.0)
        destination = get_pt_object(type_pb2.ADDRESS, 2.0, 2.0)
        valhalla._format_url("bob", origin, destination)
    assert '400: Bad Request' == str(excinfo.value)
    assert 'InvalidArguments' == str(excinfo.typename)
