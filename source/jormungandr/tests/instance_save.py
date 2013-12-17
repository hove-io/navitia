from instance_test import InstanceTest
import os
import jormungandr.request_pb2
import logging


class InstanceSave(InstanceTest):

    def send_and_receive(self, *args, **kwargs):
        resp = super(InstanceSave, self).send_and_receive(*args, **kwargs)
        request = None
        if "request" in kwargs:
            request = kwargs["request"]
        else:
            for arg in args:
                if type(arg) == jormungandr.request_pb2.Request:
                    request = arg
        if request:
            self.save(request, resp)
        else:
            logging.error("no request")
        return resp

    def save(self, request, resp):
        if not os.path.exists(self.saving_directory):
            logging.error("Saving directory" + self.saving_directory +
                          " doesn't exists")
            return
        file_ = open(self.make_filename(request), 'w+')
        file_.write(resp.SerializeToString())
        file_.close()
