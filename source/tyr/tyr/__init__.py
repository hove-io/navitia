#coding: utf-8

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

from flask import Flask
import flask_restful
from tyr.helper import configure_logger, make_celery
from redis import Redis
from celery.signals import setup_logging
from flask_script import Manager

app = Flask(__name__)
app.config.from_object('tyr.default_settings')
app.config.from_envvar('TYR_CONFIG_FILE', silent=True)
configure_logger(app)
manager = Manager(app)

#we don't want celery to mess with our logging configuration
@setup_logging.connect
def celery_setup_logging(*args, **kwargs):
    pass

if app.config['REDIS_PASSWORD']:
    app.config['CELERY_RESULT_BACKEND'] = \
        'redis://:%s@%s:%s/%s'% (app.config['REDIS_PASSWORD'],
                                app.config['REDIS_HOST'],
                                app.config['REDIS_PORT'],
                                app.config['REDIS_DB'])
else:
    app.config['CELERY_RESULT_BACKEND'] = \
        'redis://@%s:%s/%s'% (app.config['REDIS_HOST'],
                              app.config['REDIS_PORT'],
                              app.config['REDIS_DB'])


from navitiacommon.models import db
db.init_app(app)

api = flask_restful.Api(app, catch_all_404s=True)
celery = make_celery(app)

redis = Redis(host=app.config['REDIS_HOST'],
        port=app.config['REDIS_PORT'], db=app.config['REDIS_DB'],
        password=app.config['REDIS_PASSWORD'])

import tyr.api
