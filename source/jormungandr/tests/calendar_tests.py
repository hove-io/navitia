from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


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


@dataset(["main_ptref_test"])
class TestCalendar(AbstractTestFixture):

    def test_calendars(self):
        json_response = self.query_region("calendars/")

        calendars = get_not_null(json_response, "calendars")

        #we need at least one calendar
        assert calendars

        cal = calendars[0]
        check_valid_calendar(cal)

    def test_calendars_lines(self):
        json_response = self.query_region("calendars/monday/lines")

        lines = get_not_null(json_response, "lines")

        assert lines

    def test_lines_calendars(self):
        json_response = self.query_region("calendars/monday/lines/line:A/calendars")

        calendars = get_not_null(json_response, "calendars")

        assert calendars
        check_valid_calendar(calendars[0])


