from datetime import datetime
import geojson
import json
import flask_restful

def datetime_format(value):
    """Parse a valid looking date in the format YYYYmmddTHHmmss"""

    return datetime.strptime(value, "%Y%m%dT%H%M%SZ")


def json_format(value):
    if value:
        features= value.get('features')
        for feature in features:
            geometry = feature.get('geometry')
            geometry_str = json.dumps(geometry)
            valid = geojson.is_valid(geojson.loads(geometry_str))
            if valid['valid'] == 'no':
                flask_restful.abort(400, message=valid['message'])
    return value
