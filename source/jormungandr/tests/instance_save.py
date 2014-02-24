from utils import *
import os
from navitiacommon import request_pb2
import logging
from jormungandr.instance import Instance


class InstanceSave(Instance):

    def __init__(self, saving_directory, instance):
        self.geom = instance.geom
        self._sockets = instance._sockets
        self.socket_path = instance.socket_path
        self.script = instance.script
        self.nb_created_socket = instance.nb_created_socket
        self.lock = instance.lock
        self.context = instance.context
        self.name = instance.name

    def send_and_receive(self, *args, **kwargs):
        resp = super(InstanceSave, self).send_and_receive(*args, **kwargs)
        request = None
        if "request" in kwargs:
            request = kwargs["request"]
        else:
            for arg in args:
                if type(arg) == request_pb2.Request:
                    request = arg
        if request:
            logging.info("saving request")
            self.save(request, resp)
        else:
            logging.error("no request")
        return resp

    def save(self, request, resp):
        if not os.path.exists(saving_directory):
            logging.error("Saving directory" + saving_directory +
                          " doesn't exists")
            return
        file_ = open(make_filename(request), 'w+')
        file_.write(resp.SerializeToString())
        file_.close()
