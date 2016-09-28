from datetime import datetime
import geojson
import json
import flask_restful

def datetime_format(value):
    """Parse a valid looking date in the format YYYYmmddTHHmmss"""

    return datetime.strptime(value, "%Y%m%dT%H%M%SZ")

def is_geometry_valid(geometry):
    geometry_str = json.dumps(geometry)
    valid = geojson.is_valid(geojson.loads(geometry_str))
    return 'valid' in valid and (valid['valid'] == 'yes' or valid['valid'] == '')

def json_format(value):
    if value:
        is_valid_object = True
        features= value.get('features')
        if not features:
            is_valid_object = is_geometry_valid(value)
        else:
            for feature in features:
                geometry = feature.get('geometry')
                is_valid_object = is_valid_object and is_geometry_valid(geometry)
        if not is_valid_object:
            raise ValueError('invalid geometry')
    return value
