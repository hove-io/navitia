from instance_test import InstanceTest
import os
import request_pb2

class InstanceSave(InstanceTest):

    def send_and_receive(self, *args, **kwargs):
        resp = super(InstanceSave, self).send_and_receive(*args, **kwargs)
        request = None
        if kwargs.has_key("request"):
            request = kwargs["request"]
        else:
            for arg in args:
                if type(arg) == request_pb2.Request:
                    request = arg
        if request:
            self.save(request, resp)
        return resp

    def save(self, request, resp):
        if not os.path.exists(self.saving_directory):
            return
        file_ = open(self.make_filename(request), 'w+')
        file_.write(resp.SerializeToString())
        file_.close()
