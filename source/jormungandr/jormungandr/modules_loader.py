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

try:
    from typing import Dict, Text, Any, Union
except ImportError:
    pass
from flask import Blueprint
from jormungandr.module_resource import ModuleResourcesManager, ModuleResource
import six


class ModulesLoader(object):
    modules = {}  # type: Dict[Text, Union[AModule, ABlueprint]]

    def __init__(self, api):
        self.api = api

    def load(self, module):
        """
        Load the routing module.
        """
        self.modules[module.name] = module

    def run(self):
        """
            Init all modules
        """
        for module_inst in self.modules.values():
            if isinstance(module_inst, AModule):
                module_inst.setup()
            elif isinstance(module_inst, ABlueprint):
                module_inst.setup()
                self.api.app.register_blueprint(module_inst, url_prefix='/' + module_inst.name)


class ABlueprint(Blueprint):
    def __init__(
        self,
        api,
        name,
        import_name,
        description=None,
        status=None,
        index_endpoint=None,
        static_folder=None,
        static_url_path=None,
        template_folder=None,
        url_prefix=None,
        subdomain=None,
        url_defaults=None,
    ):
        super(ABlueprint, self).__init__(
            name,
            import_name,
            static_folder,
            static_url_path,
            template_folder,
            url_prefix,
            subdomain,
            url_defaults,
        )
        self.api = api
        self.description = description
        self.status = status
        self.index_endpoint = index_endpoint

    def setup(self):
        raise NotImplementedError('MUST be override')


class AModule(object):
    """
        Abstract class for routing modules.
    """

    def __init__(self, api, name, description=None, status=None, index_endpoint=None):
        """

        :type api: flask_restful.Api
        """
        super(AModule, self).__init__()
        self.api = api
        self.name = name
        self.description = description
        self.status = status
        self.index_endpoint = index_endpoint
        self.module_resources_manager = ModuleResourcesManager(self)

    def setup(self):
        raise NotImplementedError('setup() MUST be override')

    def add_resource(self, resource, *urls, **kwargs):
        """
        Inject namespace into route and endpoint before adding resource
        :param resource:
        :type resource: ModuleResource
        :param urls:
        :type urls: list
        :param kwargs:
        :type kwargs:
        """
        if 'endpoint' in kwargs:
            # on python2 we got a str ascii and in pytho 3 a str unicode, it's what we want!
            kwargs['endpoint'] = str(self.name + '.' + kwargs['endpoint'])

        urls_list = list()
        for url in urls:
            urls_list.append('/' + self.name + url)
        if isinstance(resource, ModuleResource):
            self.module_resources_manager.register_resource(resource)
        self.api.add_resource(resource, *urls_list, **kwargs)

    def add_url_rule(self, rule, endpoint=None, view_func=None, **options):
        """
        Inject namespace in rules and endpoints before adding the url rule
        :param rule:
        :type rule: str
        :param endpoint:
        :type endpoint: str
        :param view_func:
        :type view_func:
        :param options:
        :type options:
        """
        rule = '/' + self.name + rule
        if endpoint and (isinstance(endpoint, six.text_type) or isinstance(endpoint, str)):
            endpoint = self.name + '.' + endpoint
        self.api.app.add_url_rule(rule, endpoint, view_func, **options)
