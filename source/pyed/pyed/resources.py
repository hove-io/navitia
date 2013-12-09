#coding: utf-8

from flask import current_app
import flask_restful
from flask_restful import fields, marshal_with

instance_fields = {'name': fields.String,
        'rt_topics': fields.List(fields.String)}


instances_fields = {'instances': fields.List(fields.Nested(instance_fields))}


class Instance(flask_restful.Resource):
    def __init__(self):
        pass

    @marshal_with(instances_fields)
    def get(self):
        return {"instances": current_app.instances.values()}
