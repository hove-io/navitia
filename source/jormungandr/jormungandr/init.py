#!/usr/bin/env python
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
import logging
import os, sys
from jormungandr import logging_utils  # Imported to be used by configuration, py3 doesn't load it...

"""
    Import in this module should be done as late as possible to prevent side effect with the monkey patching
"""


def load_configuration(app):
    app.config.from_object('jormungandr.default_settings')
    if 'JORMUNGANDR_CONFIG_FILE' in os.environ:
        app.config.from_envvar('JORMUNGANDR_CONFIG_FILE')


def logger(app):
    if 'LOGGER' in app.config:
        logging.config.dictConfig(app.config['LOGGER'])
    else:  # Default is std out
        handler = logging.StreamHandler(stream=sys.stdout)
        app.logger.addHandler(handler)
        app.logger.setLevel('INFO')


def patch_http(patch_all=False):
    logger = logging.getLogger(__name__)
    logger.info(
        "Warning! You'are patching socket with gevent, parallel http/https calling by requests is activated"
    )

    from gevent import monkey
    if patch_all:
        monkey.patch_all()
    else:
        monkey.patch_ssl()
        monkey.patch_socket()


def street_network_backends(app):
    from jormungandr.street_network.streetnetwork_backend_manager import StreetNetworkBackendManager

    if app.config['DISABLE_DATABASE']:
        return StreetNetworkBackendManager()
    else:
        from navitiacommon import models

        return StreetNetworkBackendManager(models.StreetNetworkBackend.all)


def bss_providers(app):
    from jormungandr.parking_space_availability.bss.bss_provider_manager import BssProviderManager

    if app.config['DISABLE_DATABASE']:
        return BssProviderManager(app.config['BSS_PROVIDER'])
    else:
        from navitiacommon import models

        return BssProviderManager(app.config['BSS_PROVIDER'], models.BssProvider.all)
