#include "gateway.h"
#include <curl/curl.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include "type.pb.h"


#include <google/protobuf/descriptor.h>
#include "interface.h"

std::pair<int, std::string> Navitia::query(const std::string& request){
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
            return std::make_pair(response_code, ss.str());
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
    render(request, rd, context);
    return rd;
}


void Dispatcher::operator()(webservice::RequestData& request, webservice::ResponseData& response, Pool& pool, Context& context){
    Navitia nav =  pool.next();
    std::cout << request.path << std::endl;
    std::pair<int, std::string> res;
    try{
        res = nav.query(request.path + "?" + request.raw_params);
    }catch(long& code){
        response.status_code = code;
        return;
    }
    std::unique_ptr<pbnavitia::PTRefResponse> resp(new pbnavitia::PTRefResponse());
    if(resp->ParseFromString(res.second)){
        context.pb = std::move(resp);
        context.service = Context::PTREF;
    }else{
        context.str = res.second;
        context.service = Context::BAD_RESPONSE;
        response.status_code = res.first;
    }
}


Navitia& Pool::next(){
    if(it == navitia_list.end()){
        it = navitia_list.begin();
    }
    return *it++;
}


MAKE_WEBSERVICE(Pool, Worker)
