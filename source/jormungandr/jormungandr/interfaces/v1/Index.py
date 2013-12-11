# coding=utf-8
from flask import url_for
from flask.ext.restful import Resource


class Index(Resource):

    def get(self):
        response = {
            "links": [
                {"href": url_for('v1.coverage', _external=True),
                 "rel": "coverage",
                 "title": "Coverage of navitia"},
                {"href": url_for('v1.coord', lon=.0, lat=.0, _external=True),
                 # TODO find a way to display {long:lat} in the url
                 "rel": "coord",
                 "title": "Inverted geocoding for a given coordinate",
                 "templated": True
                 },
                {"href": url_for('v1.journeys', _external=True),
                 "rel": "journeys",
                 "title": "Compute journeys"}
            ]
        }
        return response, 200
