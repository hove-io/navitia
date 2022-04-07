# encoding: utf-8
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

import pytest
import requests_mock

from jormungandr.parking_space_availability.car.star import StarProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces

poi = {
    'properties': {'operator': 'Keolis Rennes', 'ref': '42'},
    'poi_type': {'name': 'parking relais', 'id': 'poi_type:amenity:parking'},
}


def car_park_space_availability_start_support_poi_test():
    """
    STAR car provider support
    """
    provider = StarProvider("fake.url", {'Keolis Rennes'}, 'toto', 42, username=None)
    assert provider.support_poi(poi)


def car_park_space_get_information_ok_test():
    parking_places = ParkingPlaces(available=4, occupied=3, available_PRM=2, occupied_PRM=0)
    star_response = {
        "records": [
            {
                "datasetid": "tco-parcsrelais-etat-tr",
                "fields": {
                    "nombreplacesdisponibles": 4,
                    "nombreplacesoccupees": 3,
                    "nombreplacesdisponiblespmr": 2,
                    "nombreplacesoccupeespmr": 0,
                    "idparc": "42",
                },
            }
        ]
    }

    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'toto', 42, username=None)
    with requests_mock.Mocker() as m:
        m.get('http://fake.url?dataset=toto', json=star_response)
        assert provider.get_informations(poi) == parking_places
        assert m.called


def car_park_space_get_information_invalid_poi_test():
    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'toto', 42, username=None)
    invalid_poi = {}
    with requests_mock.Mocker() as m:
        assert provider.get_informations(invalid_poi) is None
        assert not m.called


def car_park_space_get_information_none_response_test():
    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'toto', 42, username=None)
    with requests_mock.Mocker() as m:
        m.get('http://fake.url?dataset=toto', json=None)
        assert provider.get_informations(poi) is None
        assert m.called


def car_park_space_get_information_no_data_test():
    star_response = {"records": [{"datasetid": "tco-parcsrelais-etat-tr", "fields": {"idparc": "42"}}]}
    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'toto', 42, username=None)
    empty_parking = ParkingPlaces(available=None, occupied=None, available_PRM=None, occupied_PRM=None)
    with requests_mock.Mocker() as m:
        m.get('http://fake.url?dataset=toto', json=star_response)
        assert provider.get_informations(poi) == empty_parking
        assert m.called


def car_park_space_get_information_no_parking_test():
    star_response = {"records": []}
    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'toto', 42, username=None)
    with requests_mock.Mocker() as m:
        m.get('http://fake.url?dataset=toto', json=star_response)
        assert provider.get_informations(poi) is None
        assert m.called


def car_park_space_get_information_ok_without_PMR_test():
    # Information of PRM is not provided
    parking_places = ParkingPlaces(available=4, occupied=3)
    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'toto', 42, username=None)
    star_response = {
        "records": [
            {
                "datasetid": "tco-parcsrelais-etat-tr",
                "fields": {"nombreplacesdisponibles": 4, "nombreplacesoccupees": 3, "idparc": "42"},
            }
        ]
    }

    with requests_mock.Mocker() as m:
        m.get('http://fake.url?dataset=toto', json=star_response)
        info = provider.get_informations(poi)
        assert info == parking_places
        assert hasattr(info, "available")
        assert hasattr(info, "occupied")
        assert not hasattr(info, "available_PRM")
        assert not hasattr(info, "occupied_PRM")
        assert m.called


def car_park_space_get_new_information_ok_test():
    parking_places = ParkingPlaces(
        available=4,
        occupied=6,
        available_PRM=2,
        occupied_PRM=0,
        total_places=10,
        available_ridesharing=10,
        occupied_ridesharing=20,
        available_electric_vehicle=5,
        occupied_electric_vehicle=10,
        state="OUVERT",
    )
    star_response = {
        "records": [
            {
                "datasetid": "tco-parcsrelais-new-etat-tr",
                "fields": {
                    "jrdinfosoliste": 4,
                    "capacitesoliste": 10,
                    "jrdinfopmr": 2,
                    "capacitepmr": 2,
                    "jrdinfocovoiturage": 10,
                    "capacitecovoiturage": 30,
                    "jrdinfoelectrique": 5,
                    "capaciteve": 15,
                    "etatouverture": "OUVERT",
                    "idparc": "42",
                },
            }
        ]
    }

    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'test', 42, username=None)
    with requests_mock.Mocker() as m:
        m.get('http://fake.url?dataset=test', json=star_response)
        assert provider.get_informations(poi) == parking_places
        assert m.called


def car_park_space_get_new_information_no_data_test():
    star_response = {"records": [{"datasetid": "tco-parcsrelais-new-etat-tr", "fields": {"idparc": "42"}}]}
    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'test', 42, username=None)
    empty_parking = ParkingPlaces(available=None, occupied=None, available_PRM=None, occupied_PRM=None)
    with requests_mock.Mocker() as m:
        m.get('http://fake.url?dataset=test', json=star_response)
        assert provider.get_informations(poi) == empty_parking
        assert m.called


def car_park_space_get_new_information_ok_without_PMR_test():
    # Information of PRM is not provided
    parking_places = ParkingPlaces(
        available=4,
        occupied=6,
        total_places=10,
        available_ridesharing=10,
        occupied_ridesharing=20,
        available_electric_vehicle=5,
        occupied_electric_vehicle=10,
        state="OUVERT",
    )
    provider = StarProvider("http://fake.url", {'Keolis Rennes'}, 'new-etat-test', 42, username=None)
    star_response = {
        "records": [
            {
                "datasetid": "tco-parcsrelais-new-etat-tr",
                "fields": {
                    "jrdinfosoliste": 4,
                    "capacitesoliste": 10,
                    "jrdinfocovoiturage": 10,
                    "capacitecovoiturage": 30,
                    "jrdinfoelectrique": 5,
                    "capaciteve": 15,
                    "etatouverture": "OUVERT",
                    "idparc": "42",
                },
            }
        ]
    }

    with requests_mock.Mocker() as m:
        m.get('http://fake.url?dataset=new-etat-test', json=star_response)
        info = provider.get_informations(poi)
        assert info == parking_places
        assert hasattr(info, "available")
        assert hasattr(info, "occupied")
        assert hasattr(info, "total_places")
        assert hasattr(info, "available_ridesharing")
        assert hasattr(info, "occupied_ridesharing")
        assert hasattr(info, "available_electric_vehicle")
        assert hasattr(info, "occupied_electric_vehicle")
        assert hasattr(info, "state")
        assert not hasattr(info, "available_PRM")
        assert hasattr(info, "occupied_PRM")
        assert m.called
