from datetime import datetime
import pytz

def datetime_format(value):
    """Parse a valid looking date in the format YYYYmmddTHHmmss"""
    utc=pytz.UTC

    return utc.localize(datetime.strptime(value, "%Y%m%dT%H%M%S"))

