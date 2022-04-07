# encoding: utf-8

#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from sqlalchemy.dialects.postgresql.json import JSONB
from navitiacommon.models import db, TimestampMixin
from navitiacommon import default_values
from navitiacommon.constants import ENUM_EXTERNAL_SERVICE


class ExternalService(db.Model, TimestampMixin):  # type: ignore
    id = db.Column(db.Text, primary_key=True)
    klass = db.Column(db.Text, unique=False, nullable=False)
    discarded = db.Column(db.Boolean, nullable=False, default=False)
    navitia_service = db.Column(
        db.Enum(*ENUM_EXTERNAL_SERVICE, name='navitia_service_type'),
        default=default_values.external_service,
        nullable=False,
    )
    args = db.Column(JSONB, server_default='{}')

    def __init__(self, id, json=None):
        self.id = id
        if json:
            self.from_json(json)

    def from_json(self, json):
        self.klass = json['klass']
        self.args = json['args']
        self.navitia_service = json['navitia_service'] if 'navitia_service' in json else self.navitia_service
        self.discarded = json['discarded'] if 'discarded' in json else self.discarded

    @classmethod
    def find_by_id(cls, id):
        return cls.query.filter_by(id=id).one()

    @classmethod
    def _not_discarded(cls):
        return cls.query.filter_by(discarded=False)

    @classmethod
    def all(cls):
        return cls._not_discarded().all()

    @classmethod
    def get_default(cls, navitia_service=None):
        return cls.query.filter_by(discarded=False, navitia_service=navitia_service).first()

    def last_update(self):
        return self.updated_at if self.updated_at else self.created_at
