from flask.ext.restful import reqparse

paginated_parser = reqparse.RequestParser()
paginated_parser.add_argument("startPage", type=int,
                               default=0)
paginated_parser.add_argument("count", type=int,
                               default=25)
