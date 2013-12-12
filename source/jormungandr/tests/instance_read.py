from instance_test import InstanceTest
import os
from jormungandr import response_pb2
from jormungandr import request_pb2


class InstanceRead(InstanceTest):

    def send_and_receive(self, *args, **kwargs):
        request = None
        if "request" in kwargs:
            request = kwargs["request"]
        else:
            for arg in args:
                if type(arg) == request_pb2.Request:
                    request = arg
        if request:
            pb = self.read(request)
            if pb:
                resp = response_pb2.Response()
                resp.ParseFromString(pb)
                return resp
        return None

    def read(self, request):
        if not os.path.exists(self.make_filename(request)):
            return
        file_ = open(self.make_filename(request), 'rb')
        to_return = file_.read()
        file_.close()
        return to_return
