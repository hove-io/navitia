#!/usr/bin/env python
# coding=utf-8

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

from flask_restful.representations import json
from jormungandr.interfaces.v0_routing import v0_routing
from jormungandr.interfaces.v1_routing import v1_routing
from jormungandr.interfaces.documentation import v0_documentation, \
    v1_documentation
from flask import request
from jormungandr import rest_api
from jormungandr.index import index


@rest_api.representation("text/jsonp")
@rest_api.representation("application/jsonp")
def output_jsonp(data, code, headers=None):
    resp = json.output_json(data, code, headers)
    callback = request.args.get('callback', False)
    if callback:
        resp.data = str(callback) + '(' + resp.data + ')'
    return resp

index(rest_api)
v1_routing(rest_api)
v0_routing(rest_api)
v0_documentation(rest_api)
v1_documentation(rest_api)
