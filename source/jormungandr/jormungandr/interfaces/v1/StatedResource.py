#  Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from jormungandr.resources_utils import DocumentedResource
from jormungandr.stat_manager import manage_stat_caller
from jormungandr import stat_manager
from jormungandr.quota import quota_control
from functools import wraps
from jormungandr.authentication import register_used_coverages


class StatedResource(DocumentedResource):
    def __init__(self, quota=True, *args, **kwargs):
        DocumentedResource.__init__(self, *args, **kwargs)
        self.method_decorators = {'get': []}
        self.get_decorators = self.method_decorators['get']
        # HEAD is an alias for GET, we need to have the same decorators
        self.method_decorators['head'] = self.method_decorators['get']

        self.get_decorators.append(self._stat_regions)
        if stat_manager.save_stat:
            self.get_decorators.append(manage_stat_caller(stat_manager))

        if quota:
            self.get_decorators.append(quota_control)

    def _register_interpreted_parameters(self, args):
        stat_manager.register_interpreted_parameters(args)

    def _stat_regions(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            result = f(*args, **kwargs)
            region = getattr(self, 'region', None)
            if region:
                register_used_coverages([region])
            return result

        return wrapper
