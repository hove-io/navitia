#include "data_structures.h"

namespace webservice {

    RequestMethod parse_method(const std::string & method) {
        if(method == "GET") return GET;
        if(method == "POST") return POST;
        else return UNKNOWN;
    }


    ResponseData::ResponseData(const ResponseData & resp) :
                content_type(resp.content_type),
                response(resp.response.str()),
                status_code(resp.status_code),
                charset(resp.charset)
        {
        }


    ResponseData::ResponseData() : content_type("text/plain"), status_code(200), charset("utf-8"){}
}
