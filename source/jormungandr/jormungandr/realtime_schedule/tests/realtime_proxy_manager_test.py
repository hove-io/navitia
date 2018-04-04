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
from __future__ import absolute_import
import pytest
import pytz
from jormungandr.realtime_schedule.realtime_proxy_manager import RealtimeProxyManager


def realtime_proxy_creation_test():
    """
    simple timeo proxy creation
    """
    config = [{
                  'id': 'proxy_id',
                  'class': 'jormungandr.realtime_schedule.timeo.Timeo',
                  'args': {
                      'timezone': 'Europe/Paris',
                      'service_url': 'http://custom_url.com',
                      'service_args': {
                          'serviceID': 'custom_id',
                          'EntityID': 'custom_entity',
                          'Media': 'custom_media'
                      }
                  }
              }]

    manager = RealtimeProxyManager(config)

    timeo = manager.get('proxy_id')

    assert timeo
    timeo.service_url = 'http://custom_url.com'
    timeo.service_args = {
        'serviceID': 'custom_id',
        'EntityID': 'custom_entity',
        'Media': 'custom_media'
    }


def wrong_realtime_proxy_class_test():
    """
    test with a unknown class
    No error is returned, but the proxy is not created
    """
    config = [{
                  'id': 'proxy_id',
                  'class': 'wrong_class.bob',
                  'args': {
                      'service_url': 'http://custom_url.com',
                      'service_args': {
                          'serviceID': 'custom_id',
                          'EntityID': 'custom_entity',
                          'Media': 'custom_media'
                      }
                  }
              }]

    manager = RealtimeProxyManager(config)
    assert manager.get('proxy_id') is None


def wrong_argument_test():
    """
    test with a timeo proxy, but without the mandatory param 'service_url'
    No error is returned, but the proxy is not created
    """
    config = [{
                  'id': 'proxy_id',
                  'class': 'jormungandr.realtime_schedule.timeo.Timeo',
                  'args': {
                      'timezone': 'Europe/Paris',
                      'service_args': {
                          'serviceID': 'custom_id',
                          'EntityID': 'custom_entity',
                          'Media': 'custom_media'
                      }
                  }
              }]

    manager = RealtimeProxyManager(config)
    assert manager.get('proxy_id') is None


def wrong_timezone_test():
    """
    test with a timeo proxy, but with a wrong timezone
    an error should be raised
    """
    config = [{
                  'id': 'proxy_id',
                  'class': 'jormungandr.realtime_schedule.timeo.Timeo',
                  'args': {
                      'timezone': 'bobette',
                      'service_url': 'http://custom_url.com',
                      'service_args': {
                          'serviceID': 'custom_id',
                          'EntityID': 'custom_entity',
                          'Media': 'custom_media'
                      }
                  }
              }]

    with pytest.raises(pytz.UnknownTimeZoneError):
        RealtimeProxyManager(config)  # should raise an Exception


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
                'service_args': {
                    'serviceID': 'custom_id',
                    'EntityID': 'custom_entity',
                    'Media': 'custom_media'
                }
            }
        },
        {
            'id': 'wrong',
            'class': 'wrong_class',
            'args': {}
        },
        {
            # one with no id, shouldn't be created
            'class': 'wrong_class',
            'args': {}
        }
    ]
    manager = RealtimeProxyManager(config)
    assert manager.get('proxy_id') is not None
    assert manager.get('wrong') is None
