#include "gateway.h"
#include <curl/curl.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include "type.pb.h"


#include <google/protobuf/descriptor.h>
#include "interface.h"

std::string Navitia::query(const std::string& request){
    std::stringstream ss;
    std::stringstream response;
    curlpp::Easy curl_request;

    curl_request.setOpt(new curlpp::options::WriteStream(&ss));
    curl_request.setOpt(new curlpp::options::Url(this->url + request));

    try{
        curl_request.perform();
        long response_code = curlpp::infos::ResponseCode::get(curl_request);
        if(response_code >= 200 && response_code < 300){
            //tous va bien, on renvoie le flux
            return ss.str();
        }else{
            throw response_code;
        }
    }catch(curlpp::RuntimeError e){
        throw e;
    }
}

Worker::Worker(Pool &){
    register_api("/query",boost::bind(&Worker::handle, this, _1, _2), "traite les requétes");
    register_api("/load",boost::bind(&Worker::handle, this, _1, _2), "traite les requétes");
    add_default_api();
}



webservice::ResponseData Worker::handle(webservice::RequestData& request, Pool& pool){
    webservice::ResponseData rd;
    Context context;
    dispatcher(request, rd, pool, context);
    rd.response << pb2xml((pbnavitia::PTRefResponse*)context.pb);
    rd.content_type = "text/xml";
    rd.status_code = 200;
    return rd;
}


void Dispatcher::operator()(webservice::RequestData& request, webservice::ResponseData& response, Pool& pool, Context& context){
    Navitia nav =  pool.next();
    std::cout << request.path << std::endl;
    std::string pb_response = nav.query(request.path + "?" + request.raw_params);
    pbnavitia::PTRefResponse* resp = new pbnavitia::PTRefResponse();
    resp->ParseFromString(pb_response);
    context.pb = resp;
}


Navitia& Pool::next(){
    if(it == navitia_list.end()){
        it = navitia_list.begin();
    }
    return *it++;
}


MAKE_WEBSERVICE(Pool, Worker)
