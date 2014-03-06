import os
os.environ['JORMUNGANDR_CONFIG_FILE'] = os.path.dirname(os.path.realpath(__file__)) \
    + '/integration_tests_settings.py'
from utils import *
from jormungandr import i_manager, app
from nose.tools import *
from check_utils import *

__all__ = ['TestJormun']


def mock_read_send_and_receive(*args, **kwargs):
    """
    Mock send_and_receive function for integration tests
    This just read the previously serialized file for the given request
    """
    request = None
    if "request" in kwargs:
        request = kwargs["request"]
    else:
        for arg in args:
            if type(arg) == request_pb2.Request:
                request = arg
    if request:
        pb = read(request)
        if pb:
            resp = response_pb2.Response()
            resp.ParseFromString(pb)
            return resp
    return None


def read(request):
    file_name = make_filename(request)
    assert(os.path.exists(file_name))

    file_ = open(file_name, 'rb')
    to_return = file_.read()
    file_.close()
    return to_return


def check_valid_calendar(cal):
    get_not_null(cal, "id")
    get_not_null(cal, "name")
    pattern = get_not_null(cal, "week_pattern")
    is_valid_bool(get_not_null(pattern, "monday"))  # check one field in pattern

    validity_pattern = get_not_null(cal, "validity_pattern")
    assert len(validity_pattern) > 0

    beginning_date = get_not_null(validity_pattern, "beginning_date")
    assert is_valid_date(beginning_date)

    days = get_not_null(validity_pattern, "days")
    assert is_valid_days(days)

    exceptions = get_not_null(cal, "exceptions")
    assert len(exceptions) > 0

    active_periods = get_not_null(cal, "active_periods")
    assert len(active_periods) > 0

    beg = get_not_null(active_periods[0], "begin")
    assert is_valid_date(beg)

    end = get_not_null(active_periods[0], "end")
    assert is_valid_date(end)

    #check links


class TestJormun(MockInstance):
    urls = {
        "test_index": "/v1/",
        "test_coverage": "/v1/coverage",
        "test_region": "/v1/coverage/{regions.id}",
        "test_calendars": "/v1/coverage/{regions.id}/calendars",
        "test_calendars_lines": "/v1/coverage/{regions.id}/calendars/{calendars.id}/lines",

        #for the calendar by lines, we need to ensure the line has a calendar, thus this request
        "test_lines_calendars": "/v1/coverage/{regions.id}/calendars/{calendars.id}/lines/{lines.id}/calendars",
    }

    def __init__(self, *args, **kwargs):
        i_manager.initialisation(start_ping=False)
        i_manager.stop()
        self.tester = app.test_client()

        for name, instance in i_manager.instances.iteritems():
            i_manager.instances[name].send_and_receive = mock_read_send_and_receive

    def get_url(self, name):
        raw_url = self.urls[name]
        return self.transform_url(raw_url)

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

        calendars = get_not_null(json_response, "calendars")

        #we need at least one calendar
        assert calendars

        cal = calendars[0]
        check_valid_calendar(cal)

    def test_calendars_lines(self):
        json_response = check_and_get_as_dict(self, self.get_url("test_calendars_lines"))

        lines = get_not_null(json_response, "lines")

        assert lines

    def test_lines_calendars(self):
        json_response = check_and_get_as_dict(self, self.get_url("test_lines_calendars"))

        calendars = get_not_null(json_response, "calendars")

        assert calendars
        check_valid_calendar(calendars[0])

