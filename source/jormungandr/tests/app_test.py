import werkzeug
from utils import * #useful to load the settings env var definition
from jormungandr import app
from jormungandr import i_manager
from nose.tools import *
from navitiacommon.models import db
from instance_read import *

__all__ = ['TestCase']


class TestCase:
    urls = {
        "test_index": "/v1/",
        "test_coverage": "/v1/coverage",
        "test_region": "/v1/coverage/rennes",
        "test_calendars": "/v1/coverage/rennes/calendars",
    }

    def __init__(self, *args, **kwargs):
        i_manager.initialisation(start_ping=False)
        i_manager.stop()
        self.tester = app.test_client()

        for name, instance in i_manager.instances.iteritems():
            i_manager.instances[name].send_and_receive = mock_read_send_and_receive

    def test_index(self):
        json_response = check_and_get_as_json(self, self.urls["test_index"])
        #TODO!

    def test_coverage(self):
        json_response = check_and_get_as_json(self, self.urls["test_coverage"])
        #TODO!

    def test_region(self):
        json_response = check_and_get_as_json(self, self.urls["test_region"])
        #TODO!

    def test_calendars(self):
        json_response = check_and_get_as_json(self, self.urls["test_calendars"])

        assert "calendars" in json_response

        calendars = json_response["calendars"]

        #we need at least one calendar
        assert len(calendars) >= 1

        cal = calendars[0]

        get_not_null(cal, "id")
        get_not_null(cal, "name")
        get_not_null(cal, "week_pattern")

        active_periods = get_not_null(cal, "active_periods")
        assert len(active_periods) > 0

        beg = get_not_null(active_periods[0], "begin")
        assert is_valid_date(beg)

        end = get_not_null(active_periods[0], "end")
        assert is_valid_date(end)