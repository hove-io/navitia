from flask.ext.restful import Resource
from jormungandr.stat_manager import manage_stat_caller
from jormungandr import stat_manager


class StatedResource(Resource):

    def __init__(self, *args, **kwargs):
        Resource.__init__(self, *args, **kwargs)
        self.method_decorators = []

        if True:#stat_actived:
            self.method_decorators.append(manage_stat_caller(stat_manager))