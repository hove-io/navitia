# coding=utf-8
import response_pb2


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




def ManageError(response):
    if response.error:
        if response.error.id in  [response_pb2.Error.date_out_of_bounds,
                         response_pb2.Error.no_origin,
                        response_pb2.Error.no_destination,
                        response_pb2.Error.no_origin_nor_destionation,
                        response_pb2.Error.unknown_object]:
            return response, 404
        if response.error.id in [response_pb2.Error.unable_to_parse,
                                response_pb2.Error.bad_filter,
                                response_pb2.Error.unknown_api,
                                response_pb2.Error.bad_format]:
            return response, 400
        if response.error.id == response_pb2.Error.no_solution:
            return response, 204
    return response, 200
