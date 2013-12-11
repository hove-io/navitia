from flask.ext.restful import Resource
from flask import url_for


def index(api):
    api.add_resource(Index, '/')


class Index(Resource):

    def get(self):
        return {
            "versions": [
                {
                    "value": "v0",
                    "description": "Alpha version of the api",
                    "status": "deprecated",
                    "links": [
                        {
                            "href": url_for("v0", _external=True),
                            "templated": False
                        }
                    ]
                },
                {
                    "value": "v1",
                    "description": "Current version of the api",
                    "status": "current",
                    "links": [
                        {
                            "href": url_for("v1.index", _external=True),
                            "templated": False
                        }
                    ]
                }
            ]
        }
