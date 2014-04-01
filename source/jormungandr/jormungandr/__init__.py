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

import logging
import logging.config
import os
from flask import Flask, got_request_exception
from flask.ext.restful import Api
import sys
from jormungandr.exceptions import log_exception

app = Flask(__name__)
app.config.from_object('jormungandr.default_settings')
if 'JORMUNGANDR_CONFIG_FILE' in os.environ:
    app.config.from_envvar('JORMUNGANDR_CONFIG_FILE')

if 'LOGGER' in app.config:
    logging.config.dictConfig(app.config['LOGGER'])
else:  # Default is std out
    handler = logging.StreamHandler(stream=sys.stdout)
    app.logger.addHandler(handler)
    app.logger.setLevel('INFO')

got_request_exception.connect(log_exception, app)

rest_api = Api(app, catch_all_404s=True)

from navitiacommon.models import db
db.init_app(app)
if not app.config['CACHE_DISABLED']:
    from navitiacommon.cache import init_cache
    init_cache(host=app.config['REDIS_HOST'], port=app.config['REDIS_PORT'],
               db=app.config['REDIS_DB'],
               password=app.config['REDIS_PASSWORD'],
               default_ttl=app.config['AUTH_CACHE_TTL'])


from jormungandr.instance_manager import InstanceManager
i_manager = InstanceManager()
i_manager.initialisation(start_ping=app.config['START_MONITORING_THREAD'])

from jormungandr.stat_manager import StatManager
i_stat_manager = StatManager()
i_stat_manager.init("amqp://guest:guest@localhost:5672//","navitia","stat.sender")

from jormungandr import api


def setup_package():
    i_manager.stop()
