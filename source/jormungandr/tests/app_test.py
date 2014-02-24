from utils import * #useful to load the settings env var definition
from jormungandr import app
from jormungandr import i_manager
from nose.tools import *
from navitiacommon.models import db
from instance_read import mock_read_send_and_receive

__all__ = ['TestCase']


class TestCase:
    urls = {
        "test_index": "/v1/",
        "test_coverage": "/v1/coverage",
        "test_region": "/v1/coverage/rennes",
        #"test_calendars": "/v1/coverage/rennes/calendars",
    }

    def __init__(self, *args, **kwargs):
        i_manager.initialisation(start_ping=False)
        i_manager.stop()
        self.tester = app.test_client()

        for name, instance in i_manager.instances.iteritems():
            i_manager.instances[name].send_and_receive = mock_read_send_and_receive

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

    """
    def test_calendars(self):
        self.tester = app.test_client(self)
        response = self.tester.get(self.urls["test_calendars"])
        assert response is not None

        eq_(response.status_code, 200)
        """





