import werkzeug
from utils import * #useful to load the settings env var definition
from jormungandr import app
from jormungandr import i_manager
from nose.tools import *
from navitiacommon.models import db
from instance_read import *
import re

__all__ = ['TestJormun']


place_holder_regexp = re.compile("\{(.*)\.(.*)\}")


def is_place_holder(part):
    match = place_holder_regexp.match(part)

    if not match:
        return None

    return match.group(1), match.group(2)


def check_valid_calendar(cal):
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


class TestJormun:
    urls = {
        "test_index": "/v1/",
        "test_coverage": "/v1/coverage",
        "test_region": "/v1/coverage/{regions.id}",
        "test_calendars": "/v1/coverage/{regions.id}/calendars",
        "test_calendars_lines": "/v1/coverage/{regions.id}/calendars/{calendars.id}/lines",

        #for the calendar by lines, we need to ensure the line has a calendar, thus this request
        "test_lines_calendars": "/v1/coverage/{regions.id}/calendars/{calendars.id}/lines/{lines.id}/calendars",
    }

    def get_first_elt(self, current_url, place_holder):
        assert len(place_holder) == 2

        #target is a tuple, first elt if the rel to find, second is the attribute
        target_type = place_holder[0]
        target_attribute = place_holder[1]

        response = check_and_get_as_dict(self, current_url)

        list_targets = get_not_null(response, target_type)

        assert len(list_targets) > 0

        #we grab the first elt
        first = list_targets[0]

        #and we return it's attribute
        return first[target_attribute]

    def get_url(self, name):
        raw_url = self.urls[name]

        url_parts = raw_url.split('/')

        current_url = ""
        sep = ''
        for part in url_parts:
            # if it is a place holder we fetch the elt element in the collection
            place_holder = is_place_holder(part)
            if place_holder:
                part = self.get_first_elt(current_url, place_holder)
                logging.debug("for place holder %s, %s the url part is %s" % (place_holder[0], place_holder[1], part))
            current_url += sep + part
            sep = '/'

        logging.info("requesting %s" % current_url)
        return current_url

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

        check_valid_calendar(calendars[0])
        assert calendars

