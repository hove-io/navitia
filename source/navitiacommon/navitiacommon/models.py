# encoding: utf-8
import uuid
from flask_sqlalchemy import SQLAlchemy
from navitiacommon.cache import cache
from geoalchemy2.types import Geography
from flask import current_app
from sqlalchemy.orm import load_only

db = SQLAlchemy()


class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    login = db.Column(db.Text, unique=True, nullable=False)
    email = db.Column(db.Text, unique=True, nullable=False)
    keys = db.relationship('Key', backref='user', lazy='dynamic')

    authorizations = db.relationship('Authorization', backref='user',
                                     lazy='dynamic')
    cache_prefix = "user"

    def __init__(self, login=None, email=None, keys=None, authorizations=None):
        self.login = login
        self.email = email
        if keys:
            self.keys = keys
        if authorizations:
            self.authorizations = authorizations

    def __repr__(self):
        return '<User %r>' % self.email

    def add_key(self, valid_until=None):
        """
        génére une nouvelle clé pour l'utilisateur
        et l'ajoute à sa liste de clé
        c'est à l'appelant de commit la transaction
        :return la clé généré
        """
        key = Key(valid_until=valid_until)
        key.token = str(uuid.uuid4())
        self.keys.append(key)
        db.session.add(key)
        return key

    @classmethod
    def get_from_token(cls, token, valid_until):
        cache_res = cache.get(cls.cache_prefix, token)
        if cache_res is None: # we store a tuple to be able to distinguish
        #  if we have already look for this element
            res = cls.query.join(Key).filter(Key.token == token,
                                              (Key.valid_until > valid_until)
                                              | (Key.valid_until == None))
            if res:
                cache.set(cls.cache_prefix, token, (res.first(),))
                return res.first()
            else:
                cache.set(cls.cache_prefix, token, (None,))
                return None
        else:
            return cache_res[0]

    def _has_access(self, instance_name, api_name):
        q1 = Instance.query.filter(Instance.name == instance_name,
                                   Instance.is_free == True)
        query = Instance.query.join(Authorization, Api)\
            .filter(Instance.name == instance_name,
                    Api.name == api_name,
                    Authorization.user_id == self.id).union(q1)

        return query.count() > 0

    def has_access(self, instance_name, api_name):
        key = '%d_%s_%s' % (self.id, instance_name, api_name)
        res = cache.get(self.cache_prefix, key)
        if res is None:
            res = self._has_access(instance_name, api_name)
            cache.set(self.cache_prefix, key, res)
        return res


class Key(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('user.id'),
                        nullable=False)
    token = db.Column(db.Text, unique=True, nullable=False)
    valid_until = db.Column(db.Date)

    def __init__(self, token=None, user_id=None, valid_until=None):
        self.token = token
        self.user_id = user_id
        self.valid_until = valid_until

    def __repr__(self):
        return '<Key %r>' % self.token


class Instance(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.Text, unique=True, nullable=False)
    is_free = db.Column(db.Boolean, default=False, nullable=False)

    authorizations = db.relationship('Authorization', backref='instance',
            lazy='dynamic')

    jobs = db.relationship('Job', backref='instance', lazy='dynamic')

    def __init__(self, name=None, is_free=False, authorizations=None,
                 jobs=None):
        self.name = name
        self.is_free = is_free
        if authorizations:
            self.authorizations = authorizations
        if jobs:
            self.jobs = jobs

    def __repr__(self):
        return '<Instance %r>' % self.name


class Api(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.Text, unique=True, nullable=False)

    authorizations = db.relationship('Authorization', backref='api',
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


class Job(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    task_uuid = db.Column(db.Text)
    instance_id = db.Column(db.Integer,
                            db.ForeignKey('instance.id'))

    #name is used for the ENUM name in postgreSQL
    state = db.Column(db.Enum('pending', 'running', 'done', 'failed',
                              name='job_state'))

    data_sets = db.relationship('DataSet', backref='job', lazy='dynamic',
                                cascade='delete')

    def __repr__(self):
        return '<Job %r>' % self.id

class DataSet(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    type = db.Column(db.Text, nullable=False)
    name = db.Column(db.Text, nullable=False)

    job_id = db.Column(db.Integer, db.ForeignKey('job.id'))


    def __repr__(self):
        return '<DataSet %r>' % self.id



class mixin_get_from_uri():

    @classmethod
    def get_from_uri(cls, uri):
        prefix = "uri"
        cache_res = cache.get(prefix, uri)
        if cache_res is None: # we store a tuple to be able to distinguish
        #  if we have already look for this element
            res = cls.query.filter(cls.uri == uri)
            if res:
                cache.set(prefix, uri, (res.first(),))
                return res.first()
            else:
                cache.set(prefix, uri, (None,))
                return None
        else:
            return cache_res[0]


class StopAreaInstance(db.Model):
    __tablename__ = "rel_stop_area_instance"
    object_id = db.Column(db.Integer, db.ForeignKey("stop_area.id", ondelete="CASCADE"),
                         primary_key=True)
    instance_id = db.Column(db.Integer, db.ForeignKey("instance.id", ondelete="CASCADE"),
                            primary_key=True)

    def __init__(self, stop_area_id, instance_id):
        self.object_id = stop_area_id
        self.instance_id = instance_id

    def __repr__(self):
        return '<StopAreaInstance %r, %r>' % (self.object_id, self.instance_id)



class StopArea(db.Model, mixin_get_from_uri):
    id = db.Column(db.Integer, primary_key=True)
    uri = db.Column(db.Text, nullable=False, unique=True)
    coord = db.Column(Geography(geometry_type="POINT", srid=4326,
                                spatial_index=False))
    instances = db.relationship("Instance",
                            secondary="rel_stop_area_instance",
                            backref="stop_area",
                            cascade="all",
                            passive_deletes=True)
    name = db.Column(db.Text, nullable=False)
    external_code = db.Column(db.Text, index=True)
    cls_rel_instance = StopAreaInstance


    def __init__(self, id=None, uri=None, coord=None, external_code=None,
                 name=None):
        self.id = id
        self.uri = uri
        self.coord = coord
        self.external_code = external_code
        self.name = name


    def __repr__(self):
        return '<StopArea %r>' % self.id


class StopPointInstance(db.Model):
    __tablename__ = "rel_stop_point_instance"
    object_id = db.Column(db.Integer, db.ForeignKey("stop_point.id", ondelete="CASCADE"),
                         primary_key=True)
    instance_id = db.Column(db.Integer, db.ForeignKey("instance.id", ondelete="CASCADE"),
                            primary_key=True)

    def __init__(self, stop_point_id, instance_id):
        self.object_id = stop_point_id
        self.instance_id = instance_id

    def __repr__(self):
        return '<StopPointInstance %r, %r>' % (self.object_id, self.instance_id)


class StopPoint(db.Model, mixin_get_from_uri):
    id = db.Column(db.Integer, primary_key=True)
    uri = db.Column(db.Text, nullable=False, unique=True)
    coord = db.Column(Geography(geometry_type="POINT", srid=4326,
                                spatial_index=False))
    instances = db.relationship("Instance",
                            secondary="rel_stop_point_instance",
                            backref="stop_point",
                            cascade="all",
                            passive_deletes=True)
    name = db.Column(db.Text, nullable=False)
    external_code = db.Column(db.Text, index=True)
    cls_rel_instance = StopPointInstance


    def __init__(self, id=None, uri=None, coord=None, external_code=None,
                 name=None):
        self.id = id
        self.uri = uri
        self.coord = coord
        self.external_code = external_code
        self.name = name


    def __repr__(self):
        return '<StopPoint %r>' % self.id


class PoiInstance(db.Model):
    __tablename__ = "rel_poi_instance"
    object_id = db.Column(db.Integer, db.ForeignKey("poi.id", ondelete="CASCADE"),
                         primary_key=True)
    instance_id = db.Column(db.Integer, db.ForeignKey("instance.id", ondelete="CASCADE"),
                            primary_key=True)

    def __init__(self, poi_id, instance_id):
        self.object_id = poi_id
        self.instance_id = instance_id

    def __repr__(self):
        return '<PoiInstance %r, %r>' % (self.object_id, self.instance_id)


class Poi(db.Model, mixin_get_from_uri):
    id = db.Column(db.Integer, primary_key=True)
    uri = db.Column(db.Text, nullable=False, unique=True)
    coord = db.Column(Geography(geometry_type="POINT", srid=4326,
                                spatial_index=False))
    instances = db.relationship("Instance",
                            secondary="rel_poi_instance",
                            backref="poi",
                            cascade="all",
                            passive_deletes=True)
    name = db.Column(db.Text, nullable=False)
    external_code = db.Column(db.Text, index=True)
    cls_rel_instance = PoiInstance


    def __init__(self, id=None, uri=None, coord=None, external_code=None,
                 name=None):
        self.id = id
        self.uri = uri
        self.coord = coord
        self.external_code = external_code
        self.name = name


    def __repr__(self):
        return '<Poi %r>' % self.id



class AdminInstance(db.Model):
    __tablename__ = "rel_admin_instance"
    object_id = db.Column(db.Integer, db.ForeignKey("admin.id", ondelete="CASCADE"),
                         primary_key=True)
    instance_id = db.Column(db.Integer, db.ForeignKey("instance.id", ondelete="CASCADE"),
                            primary_key=True)

    def __init__(self, admin_id, instance_id):
        self.object_id = admin_id
        self.instance_id = instance_id

    def __repr__(self):
        return '<AdminInstance %r, %r>' % (self.object_id, self.instance_id)


class Admin(db.Model, mixin_get_from_uri):
    id = db.Column(db.Integer, primary_key=True)
    uri = db.Column(db.Text, nullable=False, unique=True)
    instances = db.relationship("Instance",
                            secondary="rel_admin_instance",
                            backref="admin",
                            cascade="all",
                            passive_deletes=True)
    name = db.Column(db.Text, nullable=False)
    external_code = db.Column(db.Text, index=True)
    cls_rel_instance = AdminInstance

    def __init__(self, id=None, uri=None, coord=None, external_code=None,
                 name=None):
        self.id = id
        self.uri = uri
        self.coord = coord
        self.external_code = external_code
        self.name = name


    def __repr__(self):
        return '<Admin %r>' % self.id


class LineInstance(db.Model):
    __tablename__ = "rel_line_instance"
    object_id = db.Column(db.Integer, db.ForeignKey("line.id", ondelete="CASCADE"),
                         primary_key=True)
    instance_id = db.Column(db.Integer, db.ForeignKey("instance.id", ondelete="CASCADE"),
                            primary_key=True)

    def __init__(self, line_id, instance_id):
        self.object_id = line_id
        self.instance_id = instance_id

    def __repr__(self):
        return '<LineInstance %r, %r>' % (self.object_id, self.instance_id)


class Line(db.Model, mixin_get_from_uri):
    id = db.Column(db.Integer, primary_key=True)
    uri = db.Column(db.Text, nullable=False, unique=True)
    instances = db.relationship("Instance",
                            secondary="rel_line_instance",
                            backref="line",
                            cascade="all",
                            passive_deletes=True)
    name = db.Column(db.Text, nullable=False)
    external_code = db.Column(db.Text, index=True)
    cls_rel_instance = LineInstance


    def __init__(self, id=None, uri=None, coord=None, external_code=None,
                 name=None):
        self.id = id
        self.uri = uri
        self.coord = coord
        self.external_code = external_code
        self.name = name


    def __repr__(self):
        return '<Line %r>' % self.id


class RouteInstance(db.Model):
    __tablename__ = "rel_route_instance"
    object_id = db.Column(db.Integer, db.ForeignKey("route.id", ondelete="CASCADE"),
                         primary_key=True)
    instance_id = db.Column(db.Integer, db.ForeignKey("instance.id", ondelete="CASCADE"),
                            primary_key=True)

    def __init__(self, route_id, instance_id):
        self.object_id = route_id
        self.instance_id = instance_id

    def __repr__(self):
        return '<RouteInstance %r, %r>' % (self.object_id, self.instance_id)


class Route(db.Model, mixin_get_from_uri):
    id = db.Column(db.Integer, primary_key=True)
    uri = db.Column(db.Text, nullable=False, unique=True)
    instances = db.relationship("Instance",
                            secondary="rel_route_instance",
                            backref="route",
                            cascade="all",
                            passive_deletes=True)
    name = db.Column(db.Text, nullable=False)
    external_code = db.Column(db.Text, index=True)
    cls_rel_instance = RouteInstance


    def __init__(self, id=None, uri=None, coord=None, external_code=None,
                 name=None):
        self.id = id
        self.uri = uri
        self.coord = coord
        self.external_code = external_code
        self.name = name

    def __repr__(self):
        return '<Route %r>' % self.id


class NetworkInstance(db.Model):
    __tablename__ = "rel_network_instance"
    object_id = db.Column(db.Integer, db.ForeignKey("network.id", ondelete="CASCADE"),
                         primary_key=True)
    instance_id = db.Column(db.Integer, db.ForeignKey("instance.id", ondelete="CASCADE"),
                            primary_key=True)

    def __init__(self, network_id, instance_id):
        self.object_id = network_id
        self.instance_id = instance_id

    def __repr__(self):
        return '<NetworkInstance %r, %r>' % (self.object_id, self.instance_id)


class Network(db.Model, mixin_get_from_uri):
    id = db.Column(db.Integer, primary_key=True)
    uri = db.Column(db.Text, nullable=False, unique=True)
    instances = db.relationship("Instance",
                            secondary="rel_network_instance",
                            backref="network",
                            cascade="all",
                            passive_deletes=True)
    name = db.Column(db.Text, nullable=False)
    external_code = db.Column(db.Text, index=True)
    cls_rel_instance = NetworkInstance


    def __init__(self, id=None, uri=None, coord=None, external_code=None,
                 name=None):
        self.id = id
        self.uri = uri
        self.coord = coord
        self.external_code = external_code
        self.name = name


    def __repr__(self):
        return '<Network %r>' % self.id


def get_class_type(typename):
    if typename == 'stop_area':
        return StopArea
    elif typename == 'stop_point':
        return StopPoint
    elif typename == 'poi':
        return Poi
    elif typename == 'admin':
        return Admin
    elif typename == 'line':
        return Line
    elif typename == 'route':
        return Route
    elif typename == 'network':
        return Network
    else:
        raise ValueError("Unable to find type : %s" % self.type)

class PtObject(db.Model, mixin_get_from_uri):
    id = db.Column(db.Integer, primary_key=True)
    uri = db.Column(db.Text, nullable=False, unique=True)
    name = db.Column(db.Text, nullable=False)
    original_uri = db.Column(db.Text, index=True)
    type = db.Column(db.Text, nullable=False)
    __tablename__ = "ptobject"

    def instances(self):
        cls_object = get_class_type(self.type)
        rel_instance = cls_object.cls_rel_instance
        query_rel = rel_instance.query.options(load_only("instance_id")).\
            filter(rel_instance.object_id == self.id).subquery()
        query_instance = Instance.query.filter(Instance.id.in_(query_rel))
        print query_instance
        if query_instance:
            return query_instance.all()
        else:
            return None
    @classmethod
    def get_from_external_code(cls, external_code):
        prefix = "external_code"
        cache_res = cache.get(prefix, external_code)
        if cache_res is None: # we store a tuple to be able to distinguish
        #  if we have already look for this element
            res = cls.query.filter(cls.external_code == external_code)
            if res:
                cache.set(prefix, external_code, (res.first(),))
                return res.first()
            else:
                cache.set(prefix, external_code, (None,))
                return None
        else:
            return cache_res[0]
