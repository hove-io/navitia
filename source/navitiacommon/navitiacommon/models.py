# encoding: utf-8
import uuid
from flask_sqlalchemy import SQLAlchemy
from navitiacommon.cache import cache


db = SQLAlchemy()


class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    login = db.Column(db.Text, unique=True, nullable=False)
    email = db.Column(db.Text, unique=True, nullable=False)
    keys = db.relationship('Key', backref='user', lazy='dynamic')

    authorizations = db.relationship('Authorization', backref='user',
                                     lazy='dynamic')

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
        cache_res = cache.get(token)
        if cache_res is None: # we store a tuple to be able to distinguish
        #  if we have already look for this element
            res = cls.query.join(Key).filter(Key.token == token,
                                              (Key.valid_until > valid_until)
                                              | (Key.valid_until == None))
            if res:
                cache.set(token, (res.first(),), app.config['AUTH_CACHE_TTL'])
                return res.first()
            else:
                cache.set(token, (None,), app.config['AUTH_CACHE_TTL'])
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
        res = cache.get(key)
        if res is None:
            res = self._has_access(instance_name, api_name)
            cache.set(key, res, app.config['AUTH_CACHE_TTL'])
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

    def __init__(self, name=None, is_free=False, authorizations=None):
        self.name = name
        self.is_free = is_free
        if authorizations:
            self.authorizations = authorizations

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
    filename = db.Column(db.Text)
    type = db.Column(db.Text)
    instance_id = db.Column(db.Integer,
                            db.ForeignKey('instance.id'))


