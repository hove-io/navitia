#!/usr/bin/env python
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

from tyr import resources

from tyr import app, api

api.add_resource(resources.Instance, '/v0/instances/')
api.add_resource(resources.Api, '/v0/api/')

api.add_resource(resources.User, '/v0/users/',
        '/v0/users/<int:user_id>/')

api.add_resource(resources.Key, '/v0/users/<int:user_id>/keys/',
        '/v0/users/<int:user_id>/keys/<int:key_id>/')

api.add_resource(resources.Authorization,
        '/v0/users/<int:user_id>/authorizations/')

api.add_resource(resources.Index, '/')
api.add_resource(resources.Job, '/v0/jobs/', '/v0/jobs/<string:instance_name>/', endpoint='jobs')



@app.errorhandler(Exception)
def error_handler(exception):
    """
    se charge de générer la réponse d'erreur au bon format
    """
    app.logger.exception('')
