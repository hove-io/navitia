#include "interface.h"
#include "type/pb_utils.h"

namespace navitia{ namespace gateway{

void render(webservice::RequestData& request, webservice::ResponseData& response, Context& context){
    switch(context.service){
        case Context::QUERY:
            if(request.params["format"] == "json"){
                response.response << pb2json(context.pb.get(),0);
                response.content_type = "text/plain";
                response.status_code = 200;
            }else {
                //par défaut xml
                response.response << pb2xml(context.pb.get());
                response.content_type = "text/xml";
                response.status_code = 200;
            }
            break;
        case Context::BAD_RESPONSE:
            response.response << context.str;
            break;
        default:
            response.response << "<error/>";
            response.content_type = "text/xml";
    }
}

//on recopie le pool, de cette facon le vecteur de pointeur n'est pas modifié pendant l'execution
void render_status(webservice::RequestData&, webservice::ResponseData& response, Context&, Pool pool){
    response.response << "<GatewayStatus><NavitiaList Count=\"" << pool.navitia_list.size() << "\">";
    BOOST_FOREACH(auto nav, pool.navitia_list){
        response.response << "<Navitia unusedThread=\"" << nav->unused_thread << "\" currentRequest=\"";
        response.response << nav->current_thread <<  "\">" << nav->url << "</Navitia>";
    }
    response.response << "</NavitiaList></GatewayStatus>";
    response.content_type = "text/xml";
}

}}
