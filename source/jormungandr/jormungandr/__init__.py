# encoding: utf-8

#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
import logging.config
from flask import Flask, got_request_exception
from flask_restful import Api
from flask_caching import Cache
from flask_cors import CORS
from jormungandr import init

app = Flask(__name__)  # type: Flask

init.load_configuration(app)
init.logger(app)

# we want to patch gevent as early as possible
if app.config.get(str('PATCH_WITH_GEVENT_SOCKET'), False):
    init.patch_http(patch_all=app.config.get(str('PATCH_WITH_GEVENT_SOCKET_ALL'), False))

from jormungandr import new_relic

new_relic.init(app.config.get(str('NEWRELIC_CONFIG_PATH'), None))

from jormungandr.exceptions import log_exception
from jormungandr.helper import ReverseProxied, NavitiaRequest, NavitiaRule
from jormungandr import compat, utils

app.url_rule_class = NavitiaRule
app.request_class = NavitiaRequest
CORS(
    app,
    vary_headers=True,
    allow_credentials=True,
    send_wildcard=False,
    headers=['Access-Control-Request-Headers', 'Authorization'],
)
app.config[str('CORS_HEADERS')] = 'Content-Type'


app.wsgi_app = ReverseProxied(app.wsgi_app)  # type: ignore
got_request_exception.connect(log_exception, app)

# we want the old behavior for reqparse
compat.patch_reqparse()

rest_api = Api(app, catch_all_404s=True, serve_challenge_on_401=True)

from navitiacommon.models import db

# NOTE: no db request should be made outside of a request context
# Therefore, the db app shouldn't be set here in order not to cause idle_in_transaction deadlock
db.init_app(app)
cache = Cache(app, config=app.config[str('CACHE_CONFIGURATION')])  # type: Cache
memory_cache = Cache(app, config=app.config[str('MEMORY_CACHE_CONFIGURATION')])  # type: Cache

if app.config[str('AUTOCOMPLETE_SYSTEMS')] is not None:
    global_autocomplete = {k: utils.create_object(v) for k, v in app.config[str('AUTOCOMPLETE_SYSTEMS')].items()}
else:
    from jormungandr.autocomplete.kraken import Kraken

    global_autocomplete = {'kraken': Kraken()}


streetnetwork_backend_manager = init.street_network_backends(app)

from jormungandr.instance_manager import InstanceManager

i_manager = InstanceManager(
    streetnetwork_backend_manager,
    instances_dir=app.config.get(str('INSTANCES_DIR'), None),
    instance_filename_pattern=app.config.get(str('INSTANCES_FILENAME_PATTERN'), '*.json'),
    start_ping=app.config.get(str('START_MONITORING_THREAD'), True),
)
i_manager.initialization()


from jormungandr.stat_manager import StatManager

stat_manager = StatManager()

bss_provider_manager = init.bss_providers(app)


from jormungandr.parking_space_availability.car.car_park_provider_manager import CarParkingProviderManager

car_park_provider_manager = CarParkingProviderManager(app.config[str('CAR_PARK_PROVIDER')])


from jormungandr import api


def setup_package():
    i_manager.stop()


i_manager.is_ready = True
