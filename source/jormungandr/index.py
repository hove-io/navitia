from flask.ext.restful import Resource


def index(api):
    api.add_resource(Index, '/')

class Index(Resource):
    def get(self):
        return {
        "versions" : [
            {
                "value" : "v0",
                "description" : "Alpha version of the api",
                "status" : "deprecated",
                "links" : [
                    {
                        "href" : "http://api.navitia.io/v0",
                        "templated" : False
                    }
                ]
            },
            {
                "value" : "v1",
                "description" : "Current version of the api",
                "status" : "current",
                "links" : [
                    {
                        "href" : "http://api.navitia.io/v1",
                        "templated" : False
                    }
                ]
            }
        ]
    }
