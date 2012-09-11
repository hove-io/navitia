#include "renderer.h"
#include "pb_utils.h"
#include "gateway_mod/context.h"
#include "gateway_mod/pool.h"
#include "WS_commons/data_structures.h"

namespace navitia{

void render(webservice::RequestData& request, webservice::ResponseData& response, navitia::gateway::Context& context){
    switch(context.service){
        case navitia::gateway::Context::QUERY:
            render(request, response, *context.pb);
            break;
        case navitia::gateway::Context::BAD_RESPONSE:
            response.response << context.str;
            break;
        default:
            response.response << "<error/>";
            response.content_type = "text/xml";
    }
}


void render(webservice::RequestData& request, webservice::ResponseData& response, pbnavitia::Response& pb){
    if(request.params["format"] == "pb"){
        pb.SerializeToOstream(&response.response);
        response.content_type = "application/octet-stream";
    }else if(request.params["format"] == "xml"){
        response.response << pb2xml(&pb);
        response.content_type = "text/xml";
    }else{
        response.response << pb2json(&pb, 0);
        response.content_type = "application/json";
    }
}

//on recopie le pool, de cette facon le vecteur de pointeur n'est pas modifié pendant l'execution
void render_status(webservice::RequestData&, webservice::ResponseData& response, navitia::gateway::Context&, navitia::gateway::Pool pool){
    response.response << "<GatewayStatus><NavitiaList Count=\"" << pool.navitia_list.size() << "\">";
    for(auto nav : pool.navitia_list){
        response.response << "<Navitia unusedThread=\"" << nav->unused_thread << "\" currentRequest=\"";
        response.response << nav->current_thread <<  "\">" << nav->url << "</Navitia>";
    }
    response.response << "</NavitiaList></GatewayStatus>";
    response.content_type = "text/xml";
}

}
