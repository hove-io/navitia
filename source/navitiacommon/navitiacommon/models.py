# encoding: utf-8

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

import uuid
import re
from flask_sqlalchemy import SQLAlchemy
from geoalchemy2.types import Geography
from flask import current_app
from sqlalchemy.orm import load_only, backref, aliased
from datetime import datetime
from sqlalchemy import func, and_, UniqueConstraint, cast, true, false
from sqlalchemy.dialects.postgresql import ARRAY, UUID, INTERVAL
from navitiacommon.utils import street_source_types, address_source_types, \
    poi_source_types, admin_source_types

from navitiacommon import default_values
import os

db = SQLAlchemy()


class TimestampMixin(object):
    created_at = db.Column(db.DateTime(), default=datetime.utcnow, nullable=False)
    updated_at = db.Column(db.DateTime(), default=None, onupdate=datetime.utcnow)


# https://bitbucket.org/zzzeek/sqlalchemy/issues/3467/array-of-enums-does-not-allow-assigning
class ArrayOfEnum(ARRAY):
    def bind_expression(self, bindvalue):
        return cast(bindvalue, self)

    def result_processor(self, dialect, coltype):
        super_rp = super(ArrayOfEnum, self).result_processor(dialect, coltype)

        def handle_raw_string(value):
            if value==None:
                return []
            inner = re.match(r"^{(.*)}$", value).group(1)
            return inner.split(",")

        def process(value):
            return super_rp(handle_raw_string(value))
        return process


class EndPoint(db.Model):
    """
    define a DNS name exposed by navitia, each useris associated wich one
    """
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.Text, nullable=False, unique=True)
    hosts = db.relationship('Host', backref='end_point', lazy='joined', cascade='save-update, merge, delete, delete-orphan')
    default = db.Column(db.Boolean, nullable=False, default=False)
    users = db.relationship('User', lazy='dynamic', cascade='save-update, merge, delete, delete-orphan')

    @property
    def hostnames(self):
        return [host.value for host in self.hosts]

    @classmethod
    def get_default(cls):
        return cls.query.filter(cls.default == True).first()


class Host(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    value = db.Column(db.Text, nullable=False)
    end_point_id = db.Column(db.Integer, db.ForeignKey('end_point.id'), nullable=False)

    def __init__(self, value):
        self.value = value

    def __unicode__(self):
        return self.value


class User(db.Model):
    __table_args__ = (UniqueConstraint('login', 'end_point_id', name='user_login_end_point_idx'),
                      UniqueConstraint('email', 'end_point_id', name='user_email_end_point_idx'))
    id = db.Column(db.Integer, primary_key=True)
    login = db.Column(db.Text, nullable=False)
    email = db.Column(db.Text, nullable=False)
    block_until = db.Column(db.Date, nullable=True)

    end_point_id = db.Column(db.Integer, db.ForeignKey('end_point.id'), nullable=False)
    end_point = db.relationship('EndPoint', lazy='joined', cascade='save-update, merge')

    billing_plan_id = db.Column(db.Integer, db.ForeignKey('billing_plan.id'), nullable=False)
    billing_plan = db.relationship('BillingPlan', lazy='joined', cascade='save-update, merge')

    keys = db.relationship('Key', backref='user', lazy='dynamic', cascade='save-update, merge, delete')

    authorizations = db.relationship('Authorization', backref='user',
                                     lazy='joined', cascade='save-update, merge, delete')

    type = db.Column(db.Enum('with_free_instances', 'without_free_instances', 'super_user', name='user_type'),
                             default='with_free_instances', nullable=False)

    shape = db.Column(db.Text, nullable=True)


    def __init__(self, login=None, email=None, block_until=None, keys=None, authorizations=None):
        self.login = login
        self.email = email
        self.block_until = block_until
        if keys:
            self.keys = keys
        if authorizations:
            self.authorizations = authorizations

    @property
    def have_access_to_free_instances(self):
        return self.type in ('with_free_instances', 'super_user')

    @property
    def is_super_user(self):
        return self.type == 'super_user'

    def __repr__(self):
        return '<User {}-{}>'.format(self.id, self.email)

    def add_key(self, app_name, valid_until=None):
        """
        génére une nouvelle clé pour l'utilisateur
        et l'ajoute à sa liste de clé
        c'est à l'appelant de commit la transaction
        :return la clé généré
        """
        key = Key(valid_until=valid_until)
        key.token = str(uuid.uuid4())
        key.app_name = app_name
        self.keys.append(key)
        db.session.add(key)
        return key

    @classmethod
    def get_from_token(cls, token, valid_until):
        query = cls.query.join(Key).filter(Key.token == token,
                                          (Key.valid_until > valid_until)
                                          | (Key.valid_until == None))
        res = query.first()
        return res

    def has_access(self, instance_id, api_name):
        if self.is_super_user:
            return True
        query = Instance.query.join(Authorization, Api)\
            .filter(Instance.id == instance_id,
                    Api.name == api_name,
                    Authorization.user_id == self.id)

        return query.count() > 0

    def is_blocked(self, datetime_utc):
        if self.block_until and datetime_utc <= self.block_until:
            return True

        return False

    def has_shape(self):
        return self.shape is not None

class Key(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('user.id'),
                        nullable=False)
    token = db.Column(db.Text, unique=True, nullable=False)
    app_name = db.Column(db.Text, nullable=True)
    valid_until = db.Column(db.Date)

    def __init__(self, token=None, user_id=None, valid_until=None):
        self.token = token
        self.user_id = user_id
        self.valid_until = valid_until

    def __repr__(self):
        return '<Key %r>' % self.token

    @classmethod
    def get_by_token(cls, token):
        return cls.query.filter_by(token=token).first()


class PoiTypeJson(db.Model):
    poi_types_json = db.Column(db.Text, nullable=True)
    instance_id = db.Column(db.Integer,
                            db.ForeignKey('instance.id'),
                            primary_key=True,
                            nullable=False)

    __tablename__ = 'poi_type_json'

    def __init__(self, poi_types_json, instance=None):
        self.poi_types_json = poi_types_json
        self.instance = instance


class Instance(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.Text, unique=True, nullable=False)
    discarded = db.Column(db.Boolean, default=False, nullable=False)
    #aka is_open_service in jormun
    is_free = db.Column(db.Boolean, default=False, nullable=False)

    #this doesn't impact anything but is_free was used this,
    #but an instance can be freely accessible but not using open data
    is_open_data = db.Column(db.Boolean, default=False, nullable=False)

    authorizations = db.relationship('Authorization', backref=backref('instance', lazy='joined'),
            lazy='dynamic', cascade='save-update, merge, delete')

    jobs = db.relationship('Job', backref='instance', lazy='dynamic', cascade='save-update, merge, delete')

    poi_type_json = db.relationship('PoiTypeJson', uselist=False, backref=backref('instance'),
                                 cascade='save-update, merge, delete, delete-orphan')

    import_stops_in_mimir = db.Column(db.Boolean, default=False, nullable=False)

    # ============================================================
    # params for jormungandr
    # ============================================================
    #the scenario used by jormungandr, by default we use the new default scenario (and not the default one...)
    scenario = db.Column(db.Text, nullable=False, default='new_default')

    #order of the journey, this order is for clockwise request, else it is reversed
    journey_order = db.Column(db.Enum('arrival_time', 'departure_time', name='journey_order'),
                              default=default_values.journey_order, nullable=False)

    max_walking_duration_to_pt = db.Column(db.Integer, default=default_values.max_walking_duration_to_pt, nullable=False)

    max_bike_duration_to_pt = db.Column(db.Integer, default=default_values.max_bike_duration_to_pt, nullable=False)

    max_bss_duration_to_pt = db.Column(db.Integer, default=default_values.max_bss_duration_to_pt, nullable=False)

    max_car_duration_to_pt = db.Column(db.Integer, default=default_values.max_car_duration_to_pt, nullable=False)

    walking_speed = db.Column(db.Float, default=default_values.walking_speed, nullable=False)

    bike_speed = db.Column(db.Float, default=default_values.bike_speed, nullable=False)

    bss_speed = db.Column(db.Float, default=default_values.bss_speed, nullable=False)

    car_speed = db.Column(db.Float, default=default_values.car_speed, nullable=False)

    max_nb_transfers = db.Column(db.Integer, default=default_values.max_nb_transfers, nullable=False)

    min_tc_with_car = db.Column(db.Integer, default=default_values.min_tc_with_car, nullable=False)

    min_tc_with_bike = db.Column(db.Integer, default=default_values.min_tc_with_bike, nullable=False)

    min_tc_with_bss = db.Column(db.Integer, default=default_values.min_tc_with_bss, nullable=False)

    min_bike = db.Column(db.Integer, default=default_values.min_bike, nullable=False)

    min_bss = db.Column(db.Integer, default=default_values.min_bss, nullable=False)

    min_car = db.Column(db.Integer, default=default_values.min_car, nullable=False)

    factor_too_long_journey = db.Column(db.Float, default=default_values.factor_too_long_journey, nullable=False)

    min_duration_too_long_journey = db.Column(db.Integer, default=default_values.min_duration_too_long_journey, \
            nullable=False)

    max_duration_criteria = db.Column(db.Enum('time', 'duration', name='max_duration_criteria'),
            default=default_values.max_duration_criteria, nullable=False)

    max_duration_fallback_mode = db.Column(db.Enum('walking', 'bss', 'bike', 'car', name='max_duration_fallback_mode'),
            default=default_values.max_duration_fallback_mode, nullable=False)

    max_duration = db.Column(db.Integer, default=default_values.max_duration, nullable=False, server_default='86400')

    walking_transfer_penalty = db.Column(db.Integer, default=default_values.walking_transfer_penalty, nullable=False,
                                         server_default='2')

    night_bus_filter_max_factor = db.Column(db.Float, default=default_values.night_bus_filter_max_factor,
                                            nullable=False)

    night_bus_filter_base_factor = db.Column(db.Integer, default=default_values.night_bus_filter_base_factor,
                                             nullable=False, server_default='3600')

    priority = db.Column(db.Integer, default=default_values.priority,
                                  nullable=False, server_default='0')

    bss_provider = db.Column(db.Boolean, default=default_values.bss_provider,
                                  nullable=False, server_default=true())

    max_additional_connections = db.Column(db.Integer, default=default_values.max_additional_connections,
                                  nullable=False, server_default='2')

    successive_physical_mode_to_limit_id = db.Column(db.Text,
                                                     default=default_values.successive_physical_mode_to_limit_id,
                                                     nullable=False,
                                                     server_default=default_values.successive_physical_mode_to_limit_id)

    full_sn_geometries = db.Column(db.Boolean, default=False, nullable=False, server_default=false())

    def __init__(self, name=None, is_free=False, authorizations=None,
                 jobs=None):
        self.name = name
        self.is_free = is_free
        if authorizations:
            self.authorizations = authorizations
        if jobs:
            self.jobs = jobs

    def last_datasets(self, nb_dataset=1, family_type=None):
        """
        return the n last dataset of each family type loaded for this instance
        """
        query = db.session.query(func.distinct(DataSet.family_type)) \
            .filter(Instance.id == self.id)
        if family_type:
            query = query.filter(DataSet.family_type == family_type)

        family_types = query.all()

        result = []
        for family_type in family_types:
            data_sets = db.session.query(DataSet) \
                .join(Job) \
                .join(Instance) \
                .filter(Instance.id == self.id, DataSet.family_type == family_type, Job.state == 'done') \
                .order_by(Job.created_at.desc()) \
                .limit(nb_dataset) \
                .all()
            result += data_sets
        return result

    @classmethod
    def query_existing(cls):
        return cls.query.filter_by(discarded=False)

    @classmethod
    def get_by_name(cls, name):
        res = cls.query_existing().filter_by(name=name).first()
        return res

    @classmethod
    def get_from_id_or_name(cls, id=None, name=None):
        if id:
            return cls.query.get_or_404(id)
        elif name:
            return cls.query_existing().filter_by(name=name).first_or_404()
        else:
            raise Exception({'error': 'instance is required'}, 400)

    def __repr__(self):
        return '<Instance %r>' % self.name


class TravelerProfile(db.Model):
    # http://stackoverflow.com/questions/24872541/could-not-assemble-any-primary-key-columns-for-mapped-table
    __tablename__ = 'traveler_profile'
    coverage_id = db.Column(db.Integer, db.ForeignKey('instance.id'), nullable=False)
    traveler_type = db.Column('traveler_type',
                              db.Enum('standard', 'slow_walker', 'fast_walker', 'luggage', 'wheelchair',
                                      # Temporary Profiles
                                      'cyclist', 'motorist',
                                      name='traveler_type'),
                              default='standard', nullable=False)

    __table_args__ = (db.PrimaryKeyConstraint('coverage_id', 'traveler_type'), )

    walking_speed = db.Column(db.Float, default=default_values.walking_speed, nullable=False)

    bike_speed = db.Column(db.Float, default=default_values.bike_speed, nullable=False)

    bss_speed = db.Column(db.Float, default=default_values.bss_speed, nullable=False)

    car_speed = db.Column(db.Float, default=default_values.car_speed, nullable=False)

    wheelchair = db.Column(db.Boolean, default=False, nullable=False)

    max_walking_duration_to_pt = db.Column(db.Integer, default=default_values.max_walking_duration_to_pt, nullable=False)

    max_bike_duration_to_pt = db.Column(db.Integer, default=default_values.max_bike_duration_to_pt, nullable=False)

    max_bss_duration_to_pt = db.Column(db.Integer, default=default_values.max_bss_duration_to_pt, nullable=False)

    max_car_duration_to_pt = db.Column(db.Integer, default=default_values.max_car_duration_to_pt, nullable=False)

    fallback_mode = db.Enum('walking', 'car', 'bss', 'bike', name='fallback_mode')

    first_section_mode = db.Column(ArrayOfEnum(fallback_mode), nullable=False)

    last_section_mode = db.Column(ArrayOfEnum(fallback_mode), nullable=False)

    @classmethod
    def get_by_coverage_and_type(cls, coverage, traveler_type):
        model = cls.query.join(Instance).filter(
            and_(Instance.name == coverage,
                 cls.traveler_type == traveler_type)
        ).first()
        return model

    @classmethod
    def get_all_by_coverage(cls, coverage):
        models = cls.query.join(Instance).filter(
            and_(Instance.name == coverage))
        return models


class Api(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.Text, unique=True, nullable=False)

    authorizations = db.relationship('Authorization', backref=backref('api', lazy='joined'),
                                     lazy='dynamic')

    def __init__(self, name=None):
        self.name = name

    def __repr__(self):
        return '<Api %r>' % self.name


class Authorization(db.Model):
    user_id = db.Column(db.Integer, db.ForeignKey('user.id'),
                        primary_key=True, nullable=False)
    instance_id = db.Column(db.Integer,
                            db.ForeignKey('instance.id'),
                            primary_key=True, nullable=False)
    api_id = db.Column(db.Integer,
                       db.ForeignKey('api.id'), primary_key=True,
                       nullable=False)

    def __init__(self, user_id=None, instance_id=None, api_id=None):
        self.user_id = user_id
        self.instance_id = instance_id
        self.api_id = api_id

    def __repr__(self):
        return '<Authorization %r-%r-%r>' \
                % (self.user_id, self.instance_id, self.api_id)


class Job(db.Model, TimestampMixin):
    id = db.Column(db.Integer, primary_key=True)
    task_uuid = db.Column(db.Text)
    instance_id = db.Column(db.Integer,
                            db.ForeignKey('instance.id'))
    autocomplete_params_id = db.Column(db.Integer,
                                       db.ForeignKey('autocomplete_parameter.id'))
    #name is used for the ENUM name in postgreSQL
    state = db.Column(db.Enum('pending', 'running', 'done', 'failed',
                              name='job_state'))

    data_sets = db.relationship('DataSet', backref='job', lazy='dynamic',
                                cascade='delete')

    metrics = db.relationship('Metric', backref='job', lazy='dynamic',
                                cascade='delete')

    def __repr__(self):
        return '<Job %r>' % self.id


class Metric(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    job_id = db.Column(db.Integer, db.ForeignKey('job.id'), nullable=False)

    type = db.Column(db.Enum('ed2nav', 'fusio2ed', 'gtfs2ed', 'osm2ed', 'geopal2ed', 'synonym2ed', 'poi2ed',
                             name='metric_type'), nullable=False)
    dataset_id = db.Column(db.Integer, db.ForeignKey('data_set.id'), nullable=True)
    duration = db.Column(INTERVAL)

    dataset = db.relationship('DataSet', lazy='joined')


    def __repr__(self):
        return '<Metric {}>'.format(self.id)


class DataSet(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    type = db.Column(db.Text, nullable=False)
    family_type = db.Column(db.Text, nullable=False)
    name = db.Column(db.Text, nullable=False)

    uid = db.Column(UUID, unique=True)

    job_id = db.Column(db.Integer, db.ForeignKey('job.id'))

    def __init__(self):
        self.uid = str(uuid.uuid4())

    @classmethod
    def find_by_uid(cls, uid):
        if not uid:
            #old dataset don't have uid, we don't want to get one of them
            return None
        return cls.query.filter_by(uid=uid).first()

    def __repr__(self):
        return '<DataSet %r>' % self.id


class BillingPlan(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.Text, nullable=False)
    max_request_count = db.Column(db.Integer, default=0)
    max_object_count = db.Column(db.Integer, default=0)
    default = db.Column(db.Boolean, nullable=False, default=False)

    end_point_id = db.Column(db.Integer, db.ForeignKey('end_point.id'), nullable=False)
    end_point = db.relationship('EndPoint', lazy='joined', cascade='save-update, merge')

    @classmethod
    def get_default(cls, end_point):
        return cls.query.filter_by(default=True, end_point=end_point).first()


class AutocompleteParameter(db.Model, TimestampMixin):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.Text, nullable=False, unique=True)
    street = db.Column(db.Enum(*street_source_types, name='source_street'), nullable=True)
    address = db.Column(db.Enum(address_source_types, name='source_address'), nullable=True)
    poi = db.Column(db.Enum(*poi_source_types, name='source_poi'), nullable=True)
    admin = db.Column(db.Enum(*admin_source_types, name='source_admin'), nullable=True)
    admin_level = db.Column(ARRAY(db.Integer), nullable=False)

    def __init__(self, name=None, street=None, address=None,
                 poi=None, admin=None, admin_level=None):
        self.name = name
        self.street = street
        self.address = address
        self.poi = poi
        self.admin = admin
        self.admin_level = admin_level

    def main_dir(self, root_path):
        return os.path.join(root_path, self.name)

    def source_dir(self, root_path):
        return os.path.join(self.main_dir(root_path), "source")

    def backup_dir(self, root_path):
        return os.path.join(self.main_dir(root_path), "backup")

    def last_datasets(self, nb_dataset=1):
        """
        return the n last dataset of each family type loaded for this instance
        """
        family_types = db.session.query(func.distinct(DataSet.family_type)) \
            .filter(AutocompleteParameter.id == self.id) \
            .all()

        result = []
        for family_type in family_types:
            data_sets = db.session.query(DataSet)\
                .join(Job) \
                .join(AutocompleteParameter) \
                .filter(AutocompleteParameter.id == self.id, DataSet.family_type == family_type, Job.state == 'done') \
                .order_by(Job.created_at.desc()) \
                .limit(nb_dataset) \
                .all()
            result += data_sets
        return result

    def __repr__(self):
        return '<AutocompleteParameter %r>' % self.name


