import werkzeug
from utils import * #useful to load the settings env var definition
from jormungandr import app
from jormungandr import i_manager
from nose.tools import *
from navitiacommon.models import db
from instance_read import *

__all__ = ['TestJormun']


class TestJormun:
    urls = {
        "test_index": "/v1/",
        "test_coverage": "/v1/coverage",
        "test_region": "/v1/coverage/__region_id__",
        "test_calendars": "/v1/coverage/__region_id__/calendars",
        # TODO target is
        # "test_calendars": "/v1/coverage/{region.id}/calendars/{calendars.id}/",
        # but the arrache is calling for quick and dirty patches
    }

    def load_region(self):
        # get region name
        #todo refactor this, we need a generic mechanism to get params in request
        regions = check_and_get_as_dict(self, self.urls["test_coverage"])
        self.region_name = regions["regions"][0]["id"]

    def get_url(self, name):
        raw_url = self.urls[name]
        if not self.region_name:
            self.load_region()
        url = raw_url.replace("__region_id__", self.region_name)
        return url

    def __init__(self, *args, **kwargs):
        i_manager.initialisation(start_ping=False)
        i_manager.stop()
        self.tester = app.test_client()
        self.region_name = None

        for name, instance in i_manager.instances.iteritems():
            i_manager.instances[name].send_and_receive = mock_read_send_and_receive

    def test_index(self):
        json_response = check_and_get_as_dict(self, self.get_url("test_index"))
        #TODO!

    def test_coverage(self):
        json_response = check_and_get_as_dict(self, self.get_url("test_coverage"))
        #TODO!

    def test_region(self):
        json_response = check_and_get_as_dict(self, self.get_url("test_region"))
        #TODO!

    def test_calendars(self):
        json_response = check_and_get_as_dict(self, self.get_url("test_calendars"))

        assert "calendars" in json_response

        calendars = json_response["calendars"]

        #we need at least one calendar
        assert len(calendars) >= 1

        cal = calendars[0]

        get_not_null(cal, "id")
        get_not_null(cal, "name")
        pattern = get_not_null(cal, "week_pattern")
        is_valid_bool(get_not_null(pattern, "monday"))  # check one field in pattern

        active_periods = get_not_null(cal, "active_periods")
        assert len(active_periods) > 0

        beg = get_not_null(active_periods[0], "begin")
        assert is_valid_date(beg)

        end = get_not_null(active_periods[0], "end")
        assert is_valid_date(end)

        #check links
