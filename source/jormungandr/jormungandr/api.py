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
import importlib
from flask_restful.representations import json
from flask import request, make_response
from jormungandr import rest_api, app
from jormungandr.index import index
from jormungandr.modules_loader import ModulesLoader
import ujson
import logging
from jormungandr.new_relic import record_custom_parameter
from jormungandr.authentication import get_user, get_token, get_app_name, get_used_coverages
from jormungandr._version import __version__
from jormungandr.utils import can_connect_to_database
import six


@rest_api.representation("text/jsonp")
@rest_api.representation("application/jsonp")
def output_jsonp(data, code, headers=None):
    resp = json.output_json(data, code, headers)
    callback = request.args.get('callback', False)
    if callback:
        resp.data = six.text_type(callback) + '(' + resp.data + ')'
    return resp


@rest_api.representation("text/json")
@rest_api.representation("application/json")
def output_json(data, code, headers=None):
    resp = make_response(ujson.dumps(data), code)
    resp.headers.extend(headers or {})
    return resp


@app.after_request
def access_log(response, *args, **kwargs):
    logger = logging.getLogger('jormungandr.access')
    query_string = request.query_string.decode(request.url_charset, 'replace')
    d = {
        'method': request.method,
        'path': request.path,
        'query_string': query_string,
        'status': response.status_code,
        'external_request_id': request.headers.get('x-request-id'),
    }
    logger.info(u'"%(method)s %(path)s?%(query_string)s" %(status)s', d, extra=d)
    return response


@app.after_request
def add_request_id(response, *args, **kwargs):
    response.headers['navitia-request-id'] = request.id
    return response


@app.after_request
def add_info_newrelic(response, *args, **kwargs):
    try:
        record_custom_parameter('navitia-request-id', request.id)

        if can_connect_to_database():
            token = get_token()
            user = get_user(token=token, abort_if_no_token=False)
            app_name = get_app_name(token)
            if user:
                record_custom_parameter('user_id', str(user.id))
            record_custom_parameter('token_name', app_name)

        record_custom_parameter('version', __version__)
        coverages = get_used_coverages()
        if coverages:
            record_custom_parameter('coverage', coverages[0])
    except:
        logger = logging.getLogger(__name__)
        logger.exception('error while reporting to newrelic:')
    return response


# If modules are configured, then load and run them
if 'MODULES' in rest_api.app.config:
    rest_api.module_loader = ModulesLoader(rest_api)
    for prefix, module_info in rest_api.app.config['MODULES'].items():
        module_file = importlib.import_module(module_info['import_path'])
        module = getattr(module_file, module_info['class_name'])
        rest_api.module_loader.load(module(rest_api, prefix))
    rest_api.module_loader.run()
else:
    rest_api.app.logger.warning(
        'MODULES isn\'t defined in config. No module will be loaded, then no route ' 'will be defined.'
    )

if rest_api.app.config.get('ACTIVATE_PROFILING'):
    rest_api.app.logger.warning('=======================================================')
    rest_api.app.logger.warning('activation of the profiling, all query will be slow !')
    rest_api.app.logger.warning('For better result you can do the calibration by setting PROFILING_BIAS!')
    rest_api.app.logger.warning('https://docs.python.org/2.7/library/profile.html#calibration')
    rest_api.app.logger.warning('=======================================================')
    import profile

    from werkzeug.contrib.profiler import ProfilerMiddleware

    rest_api.app.config['PROFILE'] = True
    f = open('/tmp/profiler.log', 'a')
    rest_api.app.wsgi_app = ProfilerMiddleware(
        rest_api.app.wsgi_app, f, restrictions=[80], profile_dir='/tmp/profile'
    )

index(rest_api)
