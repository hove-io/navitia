# encoding: utf-8

#  Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from datetime import timedelta


class BssProvider(db.Model, TimestampMixin):  # type: ignore
    id = db.Column(db.Text, primary_key=True)
    network = db.Column(db.Text, unique=False, nullable=False)
    klass = db.Column(db.Text, unique=False, nullable=False)
    timeout = db.Column(db.Interval, nullable=False, default=timedelta(seconds=2))
    discarded = db.Column(db.Boolean, nullable=False, default=False)
    args = db.Column(JSONB, server_default='{}')

    def __init__(self, id, json=None):
        self.id = id
        if json:
            self.from_json(json)

    def from_json(self, json):
        self.network = json['network']
        self.klass = json['klass']
        self.args = json['args']
        self.timeout = timedelta(seconds=json['timeout']) if 'timeout' in json else self.timeout
        self.discarded = json['discarded'] if 'discarded' in json else self.discarded

    def __repr__(self):
        return '<BssProvider %r>' % self.id

    @classmethod
    def _not_discarded(cls):
        return cls.query.filter_by(discarded=False)

    @classmethod
    def all(cls):
        return cls._not_discarded().all()

    @classmethod
    def find_by_id(cls, id):
        return cls.query.filter_by(id=id).one()

    def last_update(self):
        return self.updated_at if self.updated_at else self.created_at

    def full_args(self):
        """
        generate args form jormungandr implementation of a providers
        """
        timeout = self.timeout.total_seconds() if self.timeout else 2
        args = {'network': self.network, 'timeout': timeout}
        if self.args:
            args.update(self.args)
        return args
