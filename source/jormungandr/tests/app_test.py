from jormungandr import app
from jormungandr import i_manager
from instance_read import InstanceRead
from nose.tools import *
from navitiacommon.models import db


#__all__ = ['TestCase']


class TestCase:
    urls = {
        "test_index": "/v1/",
        "test_coverage": "/v1/coverage",
        "test_region": "/v1/coverage/rennes",
        "test_calendars": "/v1/coverage/rennes/calendars",
    }

    def __init__(self, *args, **kwargs):
        i_manager.stop()
        app.config.from_object('jormungandr.test_settings')
        app.config.from_envvar('JORMUNGANDR_CONFIG_FILE')
        db.drop_all()
        syncdb()
        self.tester = app.test_client()
        for name, instance in i_manager.instances.iteritems():
            i_manager.instances[name] = InstanceRead("tests/fixtures",
                                                     instance)

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

    def test_calendars(self):
        print "bob? c'est toi ?"
        eq_(True, False)
        self.tester = app.test_client(self)
        response = self.tester.get(self.urls["test_calendars"])
        eq_(response.status_code, 200)

