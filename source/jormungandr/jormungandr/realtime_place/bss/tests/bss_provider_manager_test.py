# encoding: utf-8
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
from jormungandr.realtime_place.bss.bss_provider_manager import BssProviderManager
from jormungandr import app

CONFIG = [
    {
        "class": "jormungandr.realtime_place.bss.AtosProvider",
        "args": {
            "id_ao": "10",
            "network": "Vélitul"
        }
    },
    {
        "class": "jormungandr.realtime_place.bss.AtosProvider",
        "args": {
            "id_ao": "38",
            "network": "Vlille"
        }
    }
]

def realtime_place_creation_test():
    """
    simple bss provider creation
    """
    app.config["BSS_PROVIDER"] = CONFIG
    manager = BssProviderManager()
    assert len(manager.instances) == 2

def realtime_place_bad_creation_test():
    """
    simple bss provider bad creation
    """
    app.config["BSS_PROVIDER"] = [
        {
            "class": "jormungandr.realtime_place.bss.AtosProvider",
            "args": {
                "id_ao": "10",
                "network": "Vélitul"
            }
        },
        {
            "class": "jormungandr.realtime_place.bss.BadProvider",
            "args": {
                "id_ao": "38",
                "network": "Vlille"
            }
        }
    ]
    try:
        manager = BssProviderManager()
        assert False
    except:
        assert True

def realtime_place_handle_test():
    """
    test correct handle include bss stands
    """
    places = [
        {
            "embedded_type": "poi",
            "distance": "0",
            "name": "Cit\u00e9 Universitaire (Laval)",
            "poi": {
                "properties": {
                    "network": u"Vélitul",
                    "operator": "Keolis",
                    "ref": "8"
                },
                "poi_type": {
                    "name": "station vls",
                    "id": "poi_type:amenity:bicycle_rental"
                }
            },
            "quality": 0,
            "id": "poi:n3762373698"
        }
    ]
    app.config["BSS_PROVIDER"] = CONFIG
    manager = BssProviderManager()
    places_with_stands = manager.handle_places(places)
    assert "stands" in places_with_stands[0]["poi"]

def realtime_place_find_provider_test():
    """
    test correct handle include bss stands
    """
    poi = {
        "properties": {
            "network": u"Vélitul",
            "operator": "Keolis",
            "ref": "8"
        },
        "poi_type": {
            "name": "station vls",
            "id": "poi_type:amenity:bicycle_rental"
        }
    }
    app.config["BSS_PROVIDER"] = CONFIG
    manager = BssProviderManager()
    provider = manager.find_provider(poi)
    assert provider == manager.instances[0]
    poi["properties"]["operator"] = "Bad_operator"
    provider = manager.find_provider(poi)
    assert provider is None