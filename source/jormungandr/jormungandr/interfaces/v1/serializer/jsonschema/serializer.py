# coding=utf-8

# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import
import serpy
from serpy.fields import MethodField
from jormungandr.interfaces.v1.serializer.base import LiteralField, LambdaField
from jormungandr.interfaces.v1.serializer.jsonschema.fields import Field, StrField


class SwaggerParamSerializer(serpy.Serializer):
    description = Field()
    location = Field(label='in')
    name = StrField()
    required = Field()
    type = StrField()
    default = StrField()
    enum = Field()
    minimum = Field()
    maximum = Field()
    format = Field()
    collectionFormat = Field(attr='collection_format')
    items = LambdaField(
        method=lambda _, obj: SwaggerParamSerializer(obj.items).data if obj.items else None, display_none=False
    )


class SwaggerResponseSerializer(serpy.DictSerializer):
    success = serpy.Field(attr='200', label='200')


class SwaggerMethodSerializer(serpy.Serializer):
    consumes = LiteralField(None, display_none=False)
    produces = LiteralField(["application/json"])
    responses = SwaggerResponseSerializer(attr='output_type')
    parameters = SwaggerParamSerializer(many=True)
    summary = serpy.Field()
    operationId = serpy.Field(attr='id')
    tags = serpy.Field()


class SwaggerPathSerializer(serpy.Serializer):
    # Hack since for the moment we only handle 'get' method. TODO, find a way to remove this hack
    get = MethodField(method='get_methods')

    def get_methods(self, obj):
        return SwaggerMethodSerializer(obj.methods.get('get')).data


class SwaggerOptionPathSerializer(serpy.Serializer):
    get = LambdaField(method=lambda _, obj: SwaggerMethodSerializer(obj.methods.get('get')).data)
    definitions = serpy.Field()
