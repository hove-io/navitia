# coding=utf-8

# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
# powered by Hove (www.hove.com).
# Help us simplify mobility and open public transport:
# a non ending quest to the responsive locomotion way of traveling!
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
from jormungandr.resources_utils import DocumentedResource
from flask_restful import Resource


class ModuleResourcesManager(object):
    def __init__(self, module):
        self.resources = list()
        self.module = module

    def register_resource(self, resource):
        """
            Register the resource into the ModuleResourcesManager
            Inject the module name into the resource
        :param resource:
        :type resource: ModuleResource
        :raise TypeError: Raise TypeError if resource is not an instance
                of ModuleResource
        """
        if not isinstance(resource, ModuleResource):
            raise TypeError('Expected type ModuleResource, got %s instead' % resource.__class__.__name__)
        resource.associate_to(self.module)
        self.resources.append(resource)


class ModuleResource(Resource):
    def __init__(self):
        Resource.__init__(self)
        self.method_decorators = []
        self.module_name = ''

    def associate_to(self, module):
        self.module_name = module.name

    def get(self):
        raise NotImplementedError('get() MUST be override')
