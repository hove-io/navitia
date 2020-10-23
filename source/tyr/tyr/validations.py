from datetime import datetime
from functools import wraps
from jsonschema import validate, ValidationError
from tyr.formats import parse_error
from flask_restful import abort
from flask import request
from werkzeug.exceptions import BadRequest


def datetime_format(value):
    """Parse a valid looking date in the format YYYYmmddTHHmmss"""

    return datetime.strptime(value, "%Y%m%dT%H%M%SZ")


class InputJsonValidator(object):
    """
    Check that the data received contains all required info
    """

    def __init__(self, json_format):
        self.json_format = json_format

    def __call__(self, func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            if not kwargs.get("id"):
                abort(400, status="error", message='id is required')
            try:
                input_json = request.get_json(force=True, silent=False)
                validate(input_json, self.json_format)
            except BadRequest:
                abort(400, status="error", message='Incorrect json')
            except ValidationError as e:
                abort(400, status="invalid data", message='{}'.format(parse_error(e)))
            return func(*args, **kwargs)

        return wrapper
