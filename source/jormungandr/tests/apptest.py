from jormungandr import app
from jormungandr.instance_manager import InstanceManager
from instance_read import InstanceRead
from nose.tools import *


__all__ = ['TestCase']


class TestCase:
    urls = {"test_index": "/v1/", "test_coverage": "/v1/coverage",
            "test_region": "/v1/coverage/rennes"}

    def __init__(self, *args, **kwargs):
        InstanceManager().initialisation()
        self.tester = app.test_client()
        for name, instance in InstanceManager().instances.iteritems():
            InstanceManager().instances[name] = InstanceRead("tests", instance)

    def test_index(self):
        self.tester = app.test_client(self)
        response = self.tester.get(self.urls["test_index"])
        eq_(response.status_code, 200)

    def test_coverage(self):
        self.tester = app.test_client(self)
        response = self.tester.get(self.urls["test_coverage"])
        eq_(response.status_code, 200)

    def test_region(self):
        self.tester = app.test_client(self)
        response = self.tester.get(self.urls["test_region"])
        eq_(response.status_code, 200)
