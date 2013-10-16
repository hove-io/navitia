#coding: utf-8

from flask.ext import restful
from flask import current_app
from tyr.db import get_db


def jsonify_user(users):


class Instance(restful.Resource):
    def get(self):
        return {'hello': 'world'}

class User(restful.Resource):
    def get(self):
        db = get_db('postgresql://navitia:navitia@localhost/jormun')
        with db.engine.connect() as conn:
            res = conn.execute(db.user.select())
            
