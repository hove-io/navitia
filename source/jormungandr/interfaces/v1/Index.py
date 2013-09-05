from flask import Flask, url_for
from flask.ext.restful import Resource

class Index(Resource):
    def get(self):
        response = {
                "links" : [
                    {"href"  : url_for('v1.coverage', _external=True),
                     "rel"   : "coverage",
                     "title" : "Coverage of navitia"},
                    {"href"  : "/coord/lon;lat",
                     "rel"   : "coord",
                     "title" : "Inverted geocooding" },
                    {"href"  : "v1/journeys",
                     "rel"   : "journeys",
                     "title" : "Compute journeys"}
                    ]
                }
        return response, 200
