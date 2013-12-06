#coding: utf-8

from flask import current_app
from flask.ext import restful
from flask.ext.restful import fields, marshal_with, marshal
import pyed.tasks as tasks

instance_fields = {'name': fields.String, 'rt_topics': fields.List(fields.String)}
instances_fields = {'instances': fields.List(fields.Nested(instance_fields))}

class Instance(restful.Resource):
    @marshal_with(instances_fields)
    def get(self):
        return {"instances": current_app.instances.values()}


    def post(self):
        tasks.test.delay()

