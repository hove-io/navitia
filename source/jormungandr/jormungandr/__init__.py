# encoding: utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
import logging
import logging.config
import os
from flask import Flask, got_request_exception
from flask_restful import Api
from flask.ext.cache import Cache
from flask.ext.cors import CORS
import sys
from jormungandr.exceptions import log_exception
from jormungandr.helper import ReverseProxied, NavitiaRequest
from jormungandr import compat, utils

app = Flask(__name__, static_folder=None)
app.config.from_object('jormungandr.default_settings')
if 'JORMUNGANDR_CONFIG_FILE' in os.environ:
    app.config.from_envvar('JORMUNGANDR_CONFIG_FILE')

app.request_class = NavitiaRequest
CORS(app, vary_headers=True, allow_credentials=True, send_wildcard=False,
        headers=['Access-Control-Request-Headers', 'Authorization'])
app.config['CORS_HEADERS'] = 'Content-Type'

if 'LOGGER' in app.config:
    logging.config.dictConfig(app.config['LOGGER'])
else:  # Default is std out
    handler = logging.StreamHandler(stream=sys.stdout)
    app.logger.addHandler(handler)
    app.logger.setLevel('INFO')

app.wsgi_app = ReverseProxied(app.wsgi_app)
got_request_exception.connect(log_exception, app)

#we want the old behavior for reqparse
compat.patch_reqparse()

rest_api = Api(app, catch_all_404s=True, serve_challenge_on_401=True)

from navitiacommon.models import db
db.init_app(app)
cache = Cache(app, config=app.config['CACHE_CONFIGURATION'])

if app.config['AUTOCOMPLETE_SYSTEMS'] is not None:
    global_autocomplete = {k: utils.create_object(v) for k, v in app.config['AUTOCOMPLETE_SYSTEMS'].items()}
else:
    from jormungandr.autocomplete.kraken import Kraken
    global_autocomplete = {'kraken': Kraken()}

from jormungandr.instance_manager import InstanceManager

i_manager = InstanceManager(instances_dir=app.config.get('INSTANCES_DIR', None),
                            start_ping=app.config.get('START_MONITORING_THREAD', True))
i_manager.initialisation()

from jormungandr.stat_manager import StatManager
stat_manager = StatManager()

from jormungandr.parking_space_availability.bss.bss_provider_manager import BssProviderManager
bss_provider_manager = BssProviderManager(app.config['BSS_PROVIDER'])

from jormungandr import api

def setup_package():
    i_manager.stop()
