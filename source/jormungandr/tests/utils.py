import os
import hashlib
from jormungandr.instance import Instance
import re
import logging
from check_utils import *

saving_directory = os.path.dirname(os.path.realpath(__file__)) + "/fixtures"


def make_filename(request):
    """Hash the request to make the name"""
    filename = hashlib.md5(str(request).encode()).hexdigest()
    filepath = os.path.join(saving_directory, filename)
    logging.info("for request '%s' filename '%s'" % (request, filepath))
    return filepath

place_holder_regexp = re.compile("\{(.*)\.(.*)\}")


def is_place_holder(part):
    match = place_holder_regexp.match(part)

    if not match:
        return None

    return match.group(1), match.group(2)


class MockInstance:
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

    def transform_url(self, raw_url):

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
