# encoding: utf-8

#  Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import

from sqlalchemy.dialects.postgresql.json import JSONB
from navitiacommon.models import db, TimestampMixin


class RidesharingService(db.Model, TimestampMixin):  # type: ignore
    id = db.Column(db.Text, primary_key=True)
    klass = db.Column(db.Text, unique=False, nullable=False)
    discarded = db.Column(db.Boolean, nullable=False, default=False)
    args = db.Column(JSONB, server_default='{}')

    def __init__(self, id, json=None):
        self.id = id
        if json:
            self.from_json(json)

    def from_json(self, json):
        self.klass = json['klass']
        self.args = json['args']
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

    def last_update(self):
        return self.updated_at if self.updated_at else self.created_at
