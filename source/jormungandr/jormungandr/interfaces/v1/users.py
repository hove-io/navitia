# Copyright (c) 2001-2021, Hove and/or its affiliates. All rights reserved.
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
from jormungandr.interfaces.v1.serializer.api import UserSerializer
from jormungandr.interfaces.v1.decorators import get_serializer
from jormungandr.interfaces.v1.StatedResource import StatedResource
from jormungandr import authentication
from flask_restful import abort


class User(StatedResource):
    def __init__(self, *args, **kwargs):
        super(User, self).__init__(self, *args, **kwargs)

    @get_serializer(serpy=UserSerializer)
    def get(self):
        user = authentication.get_user(token=authentication.get_token())
        if not user or user and user.login == "unknown_user":
            abort(404, message="User not found", status=404)
        return user, 200
