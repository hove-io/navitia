#include "gateway.h"
#include <curl/curl.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include "type.pb.h"

#include <boost/thread/locks.hpp>
#include <ctime>

#include <google/protobuf/descriptor.h>
#include "interface.h"

std::pair<int, std::string> Navitia::query(const std::string& request){
    std::stringstream ss;
    std::stringstream response;
    curlpp::Easy curl_request;
    
    std::cout << (this->url + request) << std::endl;

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



void Navitia::use(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    unused_thread--;
    last_request_at = time(NULL);
}

void Navitia::release(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    unused_thread++;

}

Worker::Worker(Pool &){
    register_api("/query", boost::bind(&Worker::handle, this, _1, _2), "traite les requétes");
    register_api("/load", boost::bind(&Worker::handle, this, _1, _2), "traite les requétes");
    register_api("/register", boost::bind(&Worker::register_navitia, this, _1, _2), "ajout d'un NAViTiA au pool");
    register_api("/status", boost::bind(&Worker::status, this, _1, _2), "ajout d'un NAViTiA au pool");

}

void Pool::add_navitia(Navitia* navitia){
    mutex.lock();
    navitia_list.push_back(navitia);
    std::push_heap(navitia_list.begin(), navitia_list.end(), Sorter());
    mutex.unlock();
}

webservice::ResponseData Worker::status(webservice::RequestData& request, Pool& pool){
    webservice::ResponseData rd;
    
    rd.response << "<GatewayStatus><NavitiaList Count=\"" << pool.navitia_list.size() << "\">";
    BOOST_FOREACH(Navitia* nav, pool.navitia_list){
        rd.response << "<Navitia thread=\"" << nav->unused_thread << "\">" << nav->url << "</Navitia>";
    }
    rd.response << "</NavitiaList></GatewayStatus>";
    rd.content_type = "text/xml";
    return rd;
}

webservice::ResponseData Worker::register_navitia(webservice::RequestData& request, Pool& pool){
    webservice::ResponseData rd;
    
    if(request.params.find("url") == request.params.end()){
        rd.status_code = 500;
        return rd;
    }

    //TODO valider l'url
    pool.add_navitia(new Navitia(request.params["url"], 8));
    

    return status(request, pool);
}

webservice::ResponseData Worker::handle(webservice::RequestData& request, Pool& pool){
    webservice::ResponseData rd;
    Context context;
    dispatcher(request, rd, pool, context);
    render(request, rd, context);
    return rd;
}


void Dispatcher::operator()(webservice::RequestData& request, webservice::ResponseData& response, Pool& pool, Context& context){
    Navitia* nav =  pool.next();
    std::cout << request.path << std::endl;
    std::cout << nav->url << " - " << nav->unused_thread << std::endl;

    BOOST_FOREACH(Navitia* nav, pool.navitia_list){
        std::cout << "Navitia " << nav->url << "thread= " << nav->unused_thread << std::endl;
    }
    std::pair<int, std::string> res;
    try{
        res = nav->query(request.path.substr(request.path.find_last_of('/')) + "?" + request.raw_params);
        pool.release_navitia(nav);
    }catch(long& code){
        pool.release_navitia(nav);
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




MAKE_WEBSERVICE(Pool, Worker)
