# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
from __future__ import absolute_import
import pytest
import pytz
from jormungandr.realtime_schedule.realtime_proxy_manager import RealtimeProxyManager
from navitiacommon.models import ExternalService
import datetime
from jormungandr import app


class MockInstance:
    def __init__(self, name="test1"):
        self.name = name


def realtime_proxy_creation_test():
    """
    simple timeo proxy creation
    """
    config = [
        {
            'id': 'proxy_id',
            'class': 'jormungandr.realtime_schedule.timeo.Timeo',
            'args': {
                'timezone': 'Europe/Paris',
                'service_url': 'http://custom_url.com',
                'service_args': {'serviceID': 'custom_id', 'EntityID': 'custom_entity', 'Media': 'custom_media'},
            },
        }
    ]

    manager = RealtimeProxyManager(config, MockInstance())

    timeo = manager.get('proxy_id')

    assert timeo
    timeo.service_url = 'http://custom_url.com'
    timeo.service_args = {'serviceID': 'custom_id', 'EntityID': 'custom_entity', 'Media': 'custom_media'}


def wrong_realtime_proxy_class_test():
    """
    test with a unknown class
    No error is returned, but the proxy is not created
    """
    config = [
        {
            'id': 'proxy_id',
            'class': 'wrong_class.bob',
            'args': {
                'service_url': 'http://custom_url.com',
                'service_args': {'serviceID': 'custom_id', 'EntityID': 'custom_entity', 'Media': 'custom_media'},
            },
        }
    ]

    manager = RealtimeProxyManager(config, MockInstance())
    assert manager.get('proxy_id') is None


def wrong_argument_test():
    """
    test with a timeo proxy, but without the mandatory param 'service_url'
    No error is returned, but the proxy is not created
    """
    config = [
        {
            'id': 'proxy_id',
            'class': 'jormungandr.realtime_schedule.timeo.Timeo',
            'args': {
                'timezone': 'Europe/Paris',
                'service_args': {'serviceID': 'custom_id', 'EntityID': 'custom_entity', 'Media': 'custom_media'},
            },
        }
    ]

    manager = RealtimeProxyManager(config, MockInstance())
    assert manager.get('proxy_id') is None


def wrong_timezone_test():
    """
    test with a timeo proxy, but with a wrong timezone
    an error should be raised
    """
    config = [
        {
            'id': 'proxy_id',
            'class': 'jormungandr.realtime_schedule.timeo.Timeo',
            'args': {
                'timezone': 'bobette',
                'service_url': 'http://custom_url.com',
                'service_args': {'serviceID': 'custom_id', 'EntityID': 'custom_entity', 'Media': 'custom_media'},
            },
        }
    ]

    with pytest.raises(pytz.UnknownTimeZoneError):
        RealtimeProxyManager(config, MockInstance())  # should raise an Exception


def multi_proxy_creation_test():
    """
    2 bad and one good proxy, the good one should be created
    """
    config = [
        {
            'id': 'proxy_id',
            'class': 'jormungandr.realtime_schedule.timeo.Timeo',
            'args': {
                'service_url': 'http://custom_url.com',
                'timezone': 'Europe/Paris',
                'service_args': {'serviceID': 'custom_id', 'EntityID': 'custom_entity', 'Media': 'custom_media'},
            },
        },
        {'id': 'wrong', 'class': 'wrong_class', 'args': {}},
        {
            # one with no id, shouldn't be created
            'class': 'wrong_class',
            'args': {},
        },
    ]
    manager = RealtimeProxyManager(config, MockInstance())
    assert manager.get('proxy_id') is not None
    assert manager.get('wrong') is None


All_realtime_proxies = [
    {
        "args": {"requestor_ref": "AA_Siri_TAO", "service_url": "http://siri.com", "timeout": 1},
        "class": "jormungandr.realtime_schedule.siri.Siri",
        "id": "Siri_TAO",
    },
    {
        "args": {
            "timezone": "Europe/Paris",
            "destination_id_tag": None,
            "service_url": "http://timeo.com",
            "timeout": 10,
            "service_args": {"Media": "ss_mb", "EntityID": "230", "serviceID": "4"},
        },
        "class": "jormungandr.realtime_schedule.timeo.Timeo",
        "id": "Timeo_Amelys",
    },
    {
        "object_id_tag": "source",
        "class": "jormungandr.realtime_schedule.sytral.Sytral",
        "args": {"service_url": "http://sytralrt.com", "timeout": 10},
        "id": "SytralRT",
    },
    {
        "class": "jormungandr.realtime_schedule.cleverage.Cleverage",
        "id": "clever_age_TBC",
        "args": {
            "timezone": "Europe/Paris",
            "service_url": "https://clever_age.com",
            "timeout": 1,
            "service_args": {"X-Keolis-Api-Version": "1.0", "X-Keolis-Api-Key": "HOVE_2021"},
        },
    },
    {
        "class": "jormungandr.realtime_schedule.siri_lite.SiriLite",
        "id": "siri_stif",
        "args": {"service_url": "https://siri_stif.com", "timeout": 1},
    },
]


def mock_get_realtime_proxies_from_db():
    rt_proxies = dict()
    json = {
        "navitia_service": 'realtime_proxies',
        "klass": 'jormungandr.realtime_schedule.cleverage.Cleverage',
        "args": {
            'service_url': 'https://clever_age.come',
            'timeout': 1,
            "timezone": "Europe/Paris",
            "service_args": {"X-Keolis-Api-Version": "1.0", "X-Keolis-Api-Key": "HOVE_2021"},
        },
    }
    rt_proxies["clever_age_TBC"] = ExternalService(id='clever_age_TBC', json=json)

    json = {
        "navitia_service": 'realtime_proxies',
        "klass": 'jormungandr.realtime_schedule.sytral.Sytral',
        "args": {"service_url": "http://sytralrt.com", "timeout": 10, "object_id_tag": "source"},
    }
    rt_proxies["SytralRT"] = ExternalService(id='SytralRT', json=json)

    json = {
        "navitia_service": 'realtime_proxies',
        "klass": 'jormungandr.realtime_schedule.timeo.Timeo',
        "args": {
            "timeout": 1,
            "timezone": "Europe/Paris",
            "service_url": "http://timeo.com",
            "service_args": {"Media": "ss_mb", "EntityID": "230", "serviceID": "4"},
            "destination_id": None,
        },
    }
    rt_proxies["Timeo_Amelys"] = ExternalService(id='Timeo_Amelys', json=json)

    json = {
        "navitia_service": 'realtime_proxies',
        "klass": 'jormungandr.realtime_schedule.siri.Siri',
        "args": {"requestor_ref": "AA_Siri_TAO", "service_url": "http://siri.com", "timeout": 1},
    }
    rt_proxies["Siri_TAO"] = ExternalService(id='Siri_TAO', json=json)

    json = {
        "navitia_service": 'realtime_proxies',
        "klass": 'jormungandr.realtime_schedule.siri_lite.SiriLite',
        "args": {"service_url": "https://siri_stif.com", "timeout": 1},
    }
    rt_proxies["siri_stif"] = ExternalService(id='siri_stif', json=json)

    return list(rt_proxies.values())


def multi_proxy_conf_file_and_database_test():
    manager = RealtimeProxyManager(
        All_realtime_proxies, MockInstance(), providers_getter=mock_get_realtime_proxies_from_db
    )
    assert len(manager._realtime_proxies) == 0
    assert len(manager._realtime_proxies_legacy) == 5
    for rt_proxy in ["clever_age_TBC", "SytralRT", "Timeo_Amelys", "Siri_TAO", "siri_stif"]:
        assert rt_proxy in manager._realtime_proxies_legacy

    manager.update_config()

    assert len(manager._realtime_proxies) == 5
    for rt_proxy in ["clever_age_TBC", "SytralRT", "Timeo_Amelys", "Siri_TAO", "siri_stif"]:
        assert rt_proxy in manager._realtime_proxies

    assert len(manager._realtime_proxies_legacy) == 0


def proxy_conf_file_and_database_same_id_test():
    def provider():
        json = {
            "navitia_service": 'realtime_proxies',
            "klass": 'jormungandr.realtime_schedule.sytral.Sytral',
            "args": {"service_url": "http://sytralrt.com", "timeout": 10, "object_id_tag": "source"},
        }
        return [ExternalService(id='SytralRT', json=json)]

    config = [
        {
            "object_id_tag": "source",
            "class": "jormungandr.realtime_schedule.sytral.Sytral",
            "args": {"service_url": "http://sytralrt.com", "timeout": 10},
            "id": "SytralRT",
        }
    ]
    manager = RealtimeProxyManager(config, MockInstance(), providers_getter=provider)
    assert len(manager._realtime_proxies) == 0
    assert len(manager._realtime_proxies_legacy) == 1
    assert 'SytralRT' in manager._realtime_proxies_legacy

    manager.update_config()

    assert len(manager._realtime_proxies) == 1
    assert 'SytralRT' in manager._realtime_proxies
    assert len(manager._realtime_proxies_legacy) == 0

    assert len(manager.get_all_realtime_proxies()) == 1


def one_proxy_conf_file_and_database_different_id_test():
    def provider():
        json = {
            "navitia_service": 'realtime_proxies',
            "klass": 'jormungandr.realtime_schedule.siri.Siri',
            "args": {"requestor_ref": "AA_Siri_TAO", "service_url": "http://siri.com", "timeout": 1},
        }
        return [ExternalService(id='Siri_TAO', json=json)]

    config = [
        {
            "object_id_tag": "source",
            "class": "jormungandr.realtime_schedule.sytral.Sytral",
            "args": {"service_url": "http://sytralrt.com", "timeout": 10},
            "id": "SytralRT",
        }
    ]
    manager = RealtimeProxyManager(config, MockInstance(), providers_getter=provider)
    assert len(manager._realtime_proxies) == 0
    assert len(manager._realtime_proxies_legacy) == 1
    assert 'SytralRT' in manager._realtime_proxies_legacy
    assert len(manager.get_all_realtime_proxies()) == 1

    manager.update_config()

    assert len(manager._realtime_proxies_legacy) == 1
    assert 'SytralRT' in manager._realtime_proxies_legacy

    assert len(manager._realtime_proxies) == 1
    assert 'Siri_TAO' in manager._realtime_proxies

    assert len(manager.get_all_realtime_proxies()) == 2


def proxy_only_database_test():
    def provider():
        json = {
            "navitia_service": 'realtime_proxies',
            "klass": 'jormungandr.realtime_schedule.siri.Siri',
            "args": {"requestor_ref": "AA_Siri_TAO", "service_url": "http://siri.com", "timeout": 1},
        }
        return [ExternalService(id='Siri_TAO', json=json)]

    manager = RealtimeProxyManager([], MockInstance(), providers_getter=provider)
    assert len(manager._realtime_proxies) == 0
    assert len(manager._realtime_proxies_legacy) == 0
    assert len(manager.get_all_realtime_proxies()) == 0

    manager.update_config()

    assert len(manager._realtime_proxies_legacy) == 0

    assert len(manager._realtime_proxies) == 1
    assert 'Siri_TAO' in manager._realtime_proxies
    assert len(manager.get_all_realtime_proxies()) == 1


def update_proxy_database_test():
    def provider():
        json = {
            "navitia_service": 'realtime_proxies',
            "klass": 'jormungandr.realtime_schedule.siri.Siri',
            "args": {"requestor_ref": "AA_Siri_TAO", "service_url": "http://siri.com", "timeout": 1},
        }
        external_service = ExternalService(id='Siri_TAO', json=json)
        external_service.created_at = datetime.datetime.utcnow() - datetime.timedelta(hours=2)
        return [external_service]

    manager = RealtimeProxyManager([], MockInstance(), providers_getter=provider, update_interval=-1)
    assert len(manager._realtime_proxies) == 0
    assert len(manager._realtime_proxies_legacy) == 0
    assert len(manager.get_all_realtime_proxies()) == 0

    manager.update_config()

    assert len(manager._realtime_proxies_legacy) == 0

    assert len(manager._realtime_proxies) == 1
    assert 'Siri_TAO' in manager._realtime_proxies
    assert manager._realtime_proxies["Siri_TAO"].service_url == 'http://siri.com'
    assert manager._realtime_proxies["Siri_TAO"].timeout == 1
    assert len(manager.get_all_realtime_proxies()) == 1

    # Update Url and timeout values
    def update_provider():
        json = {
            "navitia_service": 'realtime_proxies',
            "klass": 'jormungandr.realtime_schedule.siri.Siri',
            "args": {"requestor_ref": "AA_Siri_TAO", "service_url": "http://new_url_siri.com", "timeout": 10},
        }
        external_service = ExternalService(id='Siri_TAO', json=json)
        external_service.updated_at = datetime.datetime.utcnow()
        return [external_service]

    manager._realtime_proxies_getter = update_provider
    manager.update_config()

    assert len(manager._realtime_proxies_legacy) == 0

    assert len(manager._realtime_proxies) == 1
    assert 'Siri_TAO' in manager._realtime_proxies
    assert manager._realtime_proxies["Siri_TAO"].service_url == 'http://new_url_siri.com'
    assert manager._realtime_proxies["Siri_TAO"].timeout == 10
    assert len(manager.get_all_realtime_proxies()) == 1
