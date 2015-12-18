from datetime import datetime

def datetime_format(value):
    """Parse a valid looking date in the format YYYYmmddTHHmmss"""
    utc_dt = datetime.strptime(value, "%Y%m%dT%H%M%S")
    if utc_dt.year < 1900:
        raise ValueError(u"Year must be >= 1900")
    return utc_dt
