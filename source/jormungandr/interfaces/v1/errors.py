# coding=utf-8
import response_pb2
from functools import wraps
from flask import request

'''

                                Gestion des erreurs :
            ------------------------------------------------------------
            |    error_id                       |           http_code   |
            ------------------------------------------------------------
            |date_out_of_bounds					|  404                  |
            |no_origin							|  404                  |
            |no_destination						|  404                  |
            |no_origin_nor_destionation			|  404                  |
            |unknown_object						|  404                  |
            |                                   |                       |
            |no_solution                        |  204                  |
            |                                   |                       |
            |bad_filter                         |  400                  |
            |unknown_api                        |  400                  |
            |unable_to_parse                    |  400                  |
            |bad_format                         |  400                  |
            -------------------------------------------------------------

'''




class ManageError(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            response = f(*args, **kwargs)
            code = 200
            errors = {
                    response_pb2.Error.date_out_of_bounds : 404,
                    response_pb2.Error.no_origin : 404,
                    response_pb2.Error.no_destination : 404,
                    response_pb2.Error.no_origin_nor_destionation : 404,
                    response_pb2.Error.unknown_object : 404,
                    response_pb2.Error.unable_to_parse : 400,
                    response_pb2.Error.bad_filter : 400,
                    response_pb2.Error.unknown_api : 400,
                    response_pb2.Error.bad_format : 400,
                    response_pb2.Error.no_solution : 200
            }
            if response.HasField("error") and response.error.id in errors.keys():
                code = errors[response.error.id]
                if code == 400 and "filter" not in request.args.keys():
                    response.error.id = response_pb2.Error.unknown_object
                    code = 404

            else:
                response.ClearField("error")
            return response, code
        return wrapper

